#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

#include <broker/Queue.hpp>
#include <broker/DiskStorage.hpp>
#include <common/Logger.hpp>

Queue::Queue(std::string name, QueueOrder order,
    std::optional<std::chrono::seconds> messageTtl, std::string sortKey,
    std::optional<std::filesystem::path> diskDir)
    : name_(std::move(name)), order_(order), messageTtl_(messageTtl),
      sortKey_(std::move(sortKey)), diskDir_(std::move(diskDir))
{
    if (messageTtl_ && *messageTtl_ <= std::chrono::seconds::zero()) {
        throw std::invalid_argument("TTL очереди должен быть положительным");
    }
    if (order_ == QueueOrder::Sorted && sortKey_.empty()) {
        throw std::invalid_argument("Для sorted-очереди требуется sort_key");
    }

    if (diskDir_) restoreFromDisk();
}

void Queue::restoreFromDisk() {
    // создает или открывает директорию очереди
    std::filesystem::create_directories(*diskDir_);

    const auto statePath = *diskDir_ / "state.json";
    
    if (std::filesystem::exists(statePath)) {
        // читает state.json -> next_sequence и next_delivery_attempt_id
        const auto state = DiskStorage::readJson(statePath);
        if (!state.contains("next_sequence") 
            || !state["next_sequence"].is_number_unsigned()
            || !state.contains("next_delivery_attempt_id") 
            || !state["next_delivery_attempt_id"].is_number_unsigned()) {
            throw std::runtime_error("некоректен счетчик очереди: " + statePath.string());
        }

        nextSequence_ = state["next_sequence"].get<std::uint64_t>();
        nextDeliveryAttemptId_ = state["next_delivery_attempt_id"].get<std::uint64_t>();

        if (nextSequence_ == 0 || nextDeliveryAttemptId_ == 0) {
            throw std::runtime_error("некорректные счетчики очереди: " + statePath.string());
        }
    } else {
        // проход всех файлов в директории и проверка расширения
        for (const auto& file : std::filesystem::directory_iterator(*diskDir_)) {
            if (file.path().extension() == ".json") {
                throw std::runtime_error("Отсутствует state.json непустой очереди: " + diskDir_->string());
            }
        }

        // создание state.json
        saveDiskCounters(nextSequence_, nextDeliveryAttemptId_);
    }

    // обход файлов сообщений очереди
    for (const auto& file : std::filesystem::directory_iterator(*diskDir_)) {
        const auto path = file.path();
        
        if (path == statePath || path.extension() != ".json") continue;
        if (!file.is_regular_file()) throw std::runtime_error("Ожидался файл сообщения: " + path.string());
        
        try {
            const auto value = DiskStorage::readJson(path);
            if (!value.at("sequence").is_number_unsigned() 
                || !value.at("msg_id").is_string()
                || !value.at("producer_id").is_string() 
                || !value.contains("payload")
                || (!value.at("expires_at_ms").is_null() && !value.at("expires_at_ms").is_number_integer())) {
                throw std::runtime_error("неверные поля в сообщении на диске");
            }

            const auto sequence = value.at("sequence").get<std::uint64_t>();
            if (sequence == 0 
                || sequence >= nextSequence_ 
                || path.filename() != std::to_string(sequence) + ".json") {
                throw std::runtime_error("неверный sequence или имя файла на диске");
            }

            Message message{
                value.at("msg_id").get<std::string>(),
                value.at("producer_id").get<std::string>(), 
                value.at("payload")
            };
            if (message.id.empty() || message.producerId.empty()) {
                throw std::runtime_error("пустой идентификатор");
            }

            // извдечение из сообщения ключа сортировки
            std::optional<nlohmann::json> sortValue;
            if (order_ == QueueOrder::Sorted) {
                const auto field = message.payload.find(sortKey_);
                if (field == message.payload.end() || !field->is_number_integer()) {
                    throw std::runtime_error("отсутствует целый ключ сортировки");
                }

                sortValue = *field;
            }

            // проверка TTL сообщения
            std::optional<std::chrono::steady_clock::time_point> expiresAt;
            if (!value.at("expires_at_ms").is_null()) {
                const auto expiry = std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(value.at("expires_at_ms").get<std::int64_t>())
                );

                const auto systemNow = std::chrono::system_clock::now();
                
                if (expiry <= systemNow) {
                    removeDiskMessage(sequence);
                    continue;
                }
                expiresAt = std::chrono::steady_clock::now() + (expiry - systemNow);
            }

            messages_.push_back(
                Entry{SequencedMessage{sequence, std::move(message)},
                std::nullopt, 
                expiresAt, 
                std::move(sortValue)}
            );
        } catch (const std::exception& error) {
            throw std::runtime_error("Ошибка восстановления " + path.string() + ": " + error.what());
        }
    }

    std::sort(messages_.begin(), messages_.end(), 
        [](const Entry& a, const Entry& b) {
            return a.item.sequence < b.item.sequence;
        }
    );
}

void Queue::saveDiskCounters(std::uint64_t nextSequence, std::uint64_t nextDeliveryAttemptId) {
    if (diskDir_) {
        DiskStorage::writeJson(
            *diskDir_ / "state.json", 
            {
                {"next_sequence", nextSequence}, 
                {"next_delivery_attempt_id", nextDeliveryAttemptId}
            }
        );
    }
}

void Queue::removeDiskMessage(std::uint64_t sequence) {
    if (diskDir_ 
        && !std::filesystem::remove(*diskDir_ / (std::to_string(sequence) + ".json"))) {
        throw std::runtime_error("Файл сообщения не найден в очереди " + name_);
    }
}

const std::string& Queue::name() const {
    return name_;
}

QueueOrder Queue::order() const {
    return order_;
}

// -1: left < right; 0: равны; 1: left > right
int Queue::compareIntegerSortValues(const nlohmann::json& left, const nlohmann::json& right) {
    if (!left.is_number_integer() || !right.is_number_integer()) {
        throw std::logic_error("Ключи сортировки должны быть целыми числами");
    }

    auto compare = [](auto a, auto b) { 
        return (a > b) - (a < b); 
    };
    const bool leftUnsigned = left.is_number_unsigned();
    const bool rightUnsigned = right.is_number_unsigned();
    using Signed = nlohmann::json::number_integer_t;
    using Unsigned = nlohmann::json::number_unsigned_t;

    if (leftUnsigned && rightUnsigned) {
        return compare(left.get<Unsigned>(), right.get<Unsigned>());
    }
    if (!leftUnsigned && !rightUnsigned) {
        return compare(left.get<Signed>(), right.get<Signed>());
    }

    // левое число беззнаковое а другое знаковое
    if (leftUnsigned) {
        const Signed value = right.get<Signed>();
        if (value < 0) return 1; // знаковое отрицательное число меньше любого беззнакового 
        return compare(left.get<Unsigned>(), static_cast<Unsigned>(value));
    }

    const Signed value = left.get<Signed>();
    if (value < 0) return -1;
    return compare(static_cast<Unsigned>(value), right.get<Unsigned>());
}

// сравнение записей в очереди по (ключ, sequence)
bool Queue::isEntryBeforeBySortKey(const Entry& left, const Entry& right) {
    if (!left.sortValue || !right.sortValue) {
        throw std::logic_error("У записи с sorted порядком обработки сообщений отсутствует ключ сортировки");
    }

    const int keyComparison = compareIntegerSortValues(*left.sortValue, *right.sortValue);
    if (keyComparison != 0) {
        return keyComparison < 0;
    }

    return left.item.sequence < right.item.sequence;
}

/*добавление сообщения в очередь*/
void Queue::push(Message message) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()
    
    if (nextSequence_ == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Исчерпаны порядковые номера очереди: " + name_);
    }

    // поиск в payload сообщения значения поля sort_key 
    std::optional<nlohmann::json> sortValue;
    if (order_ == QueueOrder::Sorted) {
        const auto field = message.payload.find(sortKey_);
        if (field == message.payload.end() || !field->is_number_integer()) {
            throw std::invalid_argument("Для sorted-очереди требуется целочисленное поле payload: " + sortKey_);
        }
        sortValue = *field;
    }

    // момент когда TTL истечет = когда сообщение перестанет жить в очереди
    std::optional<std::chrono::steady_clock::time_point> expiresAt;
    nlohmann::json expiresAtMs = nullptr;
    if (messageTtl_) {
        // 1 для проверок во время текущей работы брокера
        expiresAt = std::chrono::steady_clock::now() + *messageTtl_;
        // 2 время сохраняемое в файл если "durability: disk"
        expiresAtMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            (std::chrono::system_clock::now() + *messageTtl_).time_since_epoch()
            ).count();
    }

    const auto sequence = nextSequence_;
    if (diskDir_) {
        // резерв sequence заранее чтобы он не повторился после сбоя или удаления
        saveDiskCounters(nextSequence_ + 1, nextDeliveryAttemptId_);
    }
    ++nextSequence_;

    messages_.push_back(
        Entry{
            SequencedMessage{sequence, std::move(message)},
            std::nullopt, // сообщение еще никому не выдано
            expiresAt,
            std::move(sortValue)
        }
    );

    if (diskDir_) {
        const Entry& entry = messages_.back();
        const auto path = *diskDir_ / (std::to_string(entry.item.sequence) + ".json");

        try {
            if (std::filesystem::exists(path)) {
                throw std::runtime_error("Файл сообщения уже существует на диске: " + path.string());
            }

            DiskStorage::writeJson(path, 
                {{"sequence", entry.item.sequence},
                {"msg_id", entry.item.message.id}, 
                {"producer_id", entry.item.message.producerId},
                {"payload", entry.item.message.payload}, 
                {"expires_at_ms", expiresAtMs}
            });
        } catch (...) {
            messages_.pop_back();
            throw;
        }
    }
} // mutex.unlock()

// (для потребителя с NoAck QoS) ищет первое письмо с номером больше lastSequence
std::optional<SequencedMessage> Queue::findNextMessageAfter(std::uint64_t lastSequence) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (order_ == QueueOrder::Sorted) {
        throw std::logic_error("Чтение sorted-очереди пока не реализовано");
    }

    removeExpiredMessagesLocked(std::chrono::steady_clock::now());

    auto it = std::upper_bound(messages_.begin(), messages_.end(), lastSequence,
        [](std::uint64_t sequence, const Entry& entry) {
            return sequence < entry.item.sequence;
        });
        
    if (it == messages_.end()) {
        return std::nullopt;
    }

    return it->item;
}

// (для потребителя с NoAck QoS и sorted order)
std::optional<SortedReadResult> Queue::findNextSortedMessage(const std::optional<SortedReadPosition>& lastPosition) {
    std::lock_guard<std::mutex> lock(mutex_);

    // if (order_ != QueueOrder::Sorted) {
    //     throw std::logic_error("Сортированное чтение доступно только для sorted-очереди");
    // }

    if (lastPosition && !lastPosition->sortValue.is_number_integer()) {
        throw std::invalid_argument("Позиция сортированного чтения должна содержать целый ключ");
    }

    // удаление писем чей TTL истек
    removeExpiredMessagesLocked(std::chrono::steady_clock::now());

    // ищет минимальную подходящую пару (ключ, sequence)
    auto best = messages_.end();
    for (auto it = messages_.begin(); it != messages_.end(); ++it) {
        if (!it->sortValue) {
            throw std::logic_error("У записи sorted-очереди отсутствует ключ сортировки");
        }
        if (lastPosition) {
            const int keyComparison = compareIntegerSortValues(*it->sortValue, lastPosition->sortValue);
            // меньший ключ нельзя отдавать; равный возможен только после последнего sequence
            if (keyComparison < 0 || (keyComparison == 0 && it->item.sequence <= lastPosition->sequence)) {
                continue;
            }
        }
        if (best == messages_.end() || isEntryBeforeBySortKey(*it, *best)) {
            best = it;
        }
    }

    if (best == messages_.end()) {
        return std::nullopt;
    }

    return SortedReadResult{
        best->item, 
        SortedReadPosition{
            *best->sortValue, /*key*/ 
            best->item.sequence /*sequence*/
        }
    };
}

// (для потребителя с WithAck QoS) выбирает сообщение из очереди и закрепляет его за указанным потребителем по clientId
std::optional<Delivery> Queue::reserveNextMessageForAck(const std::string& clientId, 
    std::chrono::milliseconds ackTimeout,
    const std::optional<SortedReadPosition>& lastSortedPosition) 
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (ackTimeout <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Таймаут подтверждения должен быть положительным");
    }
    if (order_ == QueueOrder::Sorted 
        && lastSortedPosition 
        && !lastSortedPosition->sortValue.is_number_integer()) {
        throw std::invalid_argument("Позиция сортированного чтения должна содержать целый ключ");
    }

    // удаляю просроченные письма, освобождаю просроченные выдачи потребителям
    const auto now = std::chrono::steady_clock::now();
    removeExpiredMessagesLocked(now);
    releaseExpiredDeliveriesLocked(now);

    if (messages_.empty()) {
        return std::nullopt;
    }

    // 1 - выбор письма которое можно выдать потребителю
    auto it = messages_.begin();
    if (order_ == QueueOrder::Fifo) {
        // fifo -> если первое письмо уже выдано и ждет ack то следующее письмо не выдавать
        if (it->active) {
            return std::nullopt;
        }
    } else if (order_ == QueueOrder::Sorted) {
        // среди свободных писем выбор минимальной пары (key, sequence)
        it = messages_.end();
        for (auto candidate = messages_.begin(); candidate != messages_.end(); ++candidate) {
            if (!candidate->sortValue) {
                throw std::logic_error("У записи очереди с sorted order отсутствует ключ сортировки");
            }

            // пропуск уже ожидающих ack писем || key кандидата < key lastSortedPosition
            if (candidate->active || 
                (lastSortedPosition && compareIntegerSortValues(*candidate->sortValue, lastSortedPosition->sortValue) < 0)) {
                continue;
            }

            // выбор подходящего письма - сравнение их key, затем в случае равенста sequence number в очереди
            if (it == messages_.end() || 
                isEntryBeforeBySortKey(*candidate, *it)) {
                it = candidate;
            }
        }
        if (it == messages_.end()) return std::nullopt;
    } else {
        // для unordered -> ищу первое письмо без активной выдачи (т.е. оно доступно для выдачи)
        it = std::find_if(messages_.begin(), messages_.end(),
            [](const Entry& entry) { 
                return !entry.active; 
            });
        if (it == messages_.end()) return std::nullopt;
    }

    if (nextDeliveryAttemptId_ == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Исчерпаны номера выдач очереди: " + name_);
    }

    // 2 - РЕЗЕРВИРОВАНИЕ = запись в active письма какому клиенту и в рамках какой попытки оно выдано 
    /* --- для очередей с durability: disk --- */
    const auto attemptId = nextDeliveryAttemptId_;
    saveDiskCounters(nextSequence_, nextDeliveryAttemptId_ + 1);
    ++nextDeliveryAttemptId_;
    /* --------------------------------------- */

    Delivery delivery{
        it->item.message, 
        std::to_string(attemptId),
        std::nullopt
    };
    if (order_ == QueueOrder::Sorted) {
        delivery.sortedPosition = SortedReadPosition{*it->sortValue, it->item.sequence};
    }
    it->active = ActiveDelivery{
        clientId, 
        delivery.deliveryAttemptId,
        // до какого момента брокер ожидает ack от потребителя
        std::chrono::steady_clock::now() + ackTimeout // дедлайн = текущий момент + длительность ожидания подтверждения получения сообщения потребителем
    };

    // 3 - теперь сообщение закреплено брокером за потребителем и выдано ему  
    return delivery;
}

// проверка ack и удаление письма из очереди
bool Queue::confirmDeliveryAndRemoveMessage(const std::string& clientId, 
    const std::string& messageId, 
    const std::string& deliveryAttemptId) 
{
    std::lock_guard<std::mutex> lock(mutex_);

    /* пример:
    1 A выдано потребителю с WithAck
    2 у A истек ттл -> должен быть удален из очереди
    3 ack пришел на A
    */
    removeExpiredMessagesLocked(std::chrono::steady_clock::now());

    auto it = std::find_if(
        messages_.begin(), 
        messages_.end(), 
        /*
        поиск письма в котором: 
            id письма совпал
            есть активная выдача
            identity клиента совпал
            id попытки выдачи совпал
        */
        [&](const Entry& entry) {
            return entry.item.message.id == messageId 
                && entry.active
                && entry.active->clientId == clientId 
                && entry.active->deliveryAttemptId == deliveryAttemptId;
        });
    if (it == messages_.end()) return false;

    // если периодическая проверка еще не выполнялась после дедлайна
    if (std::chrono::steady_clock::now() >= it->active->deadline) {
        it->active.reset();
        return false;
    }

    // удаление выбранного письма из очереди на диске
    removeDiskMessage(it->item.sequence);

    // удаление выбранного письма из очереди в памяти
    messages_.erase(it);

    return true;
}

std::size_t Queue::releaseExpiredDeliveriesLocked(std::chrono::steady_clock::time_point now) {
    std::size_t released = 0;
    for (Entry& entry : messages_) {
        if (entry.active 
            // срок ожидания истек?
            && now >= entry.active->deadline) {
            entry.active.reset(); // удалил пометку выдачи; НЕ само письмо
            ++released;
        }
    }

    return released;
}

std::size_t Queue::releaseExpiredDeliveries() {
    std::lock_guard<std::mutex> lock(mutex_);

    return releaseExpiredDeliveriesLocked(std::chrono::steady_clock::now());
}

std::size_t Queue::removeExpiredMessagesLocked(std::chrono::steady_clock::time_point now) {
    const auto oldSize = messages_.size();

    // удаление из messages_ всех записей для которых условие вернуло true
    for (auto it = messages_.begin(); it != messages_.end();) {
        if (it->expiresAt && now >= *it->expiresAt) {
            // удаление файла если очередь дисковая
            removeDiskMessage(it->item.sequence);
            // удаление записи из очереди на памяти
            it = messages_.erase(it);
        } else {
            ++it;
        }
    }

    const std::size_t removed = oldSize - messages_.size();
    if (removed > 0) {
        Logger::log("Очередь " + name_ + ": по TTL удалено сообщений: " + std::to_string(removed));
    }
    
    return removed;
}

std::size_t Queue::removeExpiredMessages() {
    std::lock_guard<std::mutex> lock(mutex_);

    return removeExpiredMessagesLocked(std::chrono::steady_clock::now());
}

bool Queue::isMessageAvailable(const std::string& messageId) {
    std::lock_guard<std::mutex> lock(mutex_);

    removeExpiredMessagesLocked(std::chrono::steady_clock::now());

    return std::any_of(messages_.begin(), messages_.end(), [&](const Entry& entry) {
        return entry.item.message.id == messageId;
    });
}

// удаление пометок выдачи письма при отписке клиента от очереди 
void Queue::releaseAllDeliveriesForClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (Entry& entry : messages_) {
        if (entry.active && entry.active->clientId == clientId) {
            entry.active.reset();
        }
    }
}

// отменить одно резервирование|одну активную выдачу письма клиенту при проблеме с выдачей
void Queue::releaseActiveDelivery(const std::string& clientId, const std::string& messageId, const std::string& deliveryAttemptId) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (Entry& entry : messages_) {
        if (entry.item.message.id == messageId 
            && entry.active
            && entry.active->clientId == clientId 
            && entry.active->deliveryAttemptId == deliveryAttemptId) {
            entry.active.reset();
            return;
        }
    }
}

std::size_t Queue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return messages_.size();
}
