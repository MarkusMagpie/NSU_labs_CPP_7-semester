#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

#include <broker/MemoryQueue.hpp>
#include <common/Logger.hpp>

MemoryQueue::MemoryQueue(std::string name, QueueOrder order,
    std::optional<std::chrono::seconds> messageTtl, std::string sortKey)
    : name_(std::move(name)), order_(order), messageTtl_(messageTtl), sortKey_(std::move(sortKey))
{
    if (messageTtl_ && *messageTtl_ <= std::chrono::seconds::zero()) {
        throw std::invalid_argument("TTL очереди должен быть положительным");
    }
    if (order_ == QueueOrder::Sorted && sortKey_.empty()) {
        throw std::invalid_argument("Для sorted-очереди требуется sort_key");
    }
}

const std::string& MemoryQueue::name() const {
    return name_;
}

QueueOrder MemoryQueue::order() const {
    return order_;
}

// -1: left < right; 0: равны; 1: left > right
int MemoryQueue::compareIntegerSortValues(const nlohmann::json& left, const nlohmann::json& right) {
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
bool MemoryQueue::isEntryBeforeBySortKey(const Entry& left, const Entry& right) {
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
void MemoryQueue::push(Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
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
    if (messageTtl_) {
        expiresAt = std::chrono::steady_clock::now() + *messageTtl_;
    }

    messages_.push_back(
        Entry{
            SequencedMessage{nextSequence_, std::move(message)}, 
            std::nullopt, // сообщение еще никому не выдано
            expiresAt,
            std::move(sortValue)
        }
    );

    ++nextSequence_;
}

// (для потребителя с NoAck QoS) ищет первое письмо с номером больше lastSequence
std::optional<SequencedMessage> MemoryQueue::findNextMessageAfter(std::uint64_t lastSequence) {
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
std::optional<SortedReadResult> MemoryQueue::findNextSortedMessage(const std::optional<SortedReadPosition>& lastPosition) {
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
std::optional<Delivery> MemoryQueue::reserveNextMessageForAck(const std::string& clientId, 
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
    Delivery delivery{
        it->item.message, 
        std::to_string(nextDeliveryAttemptId_),
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
    ++nextDeliveryAttemptId_;
    
    return delivery;
}

// проверка ack и удаление письма из очереди
bool MemoryQueue::confirmDeliveryAndRemoveMessage(const std::string& clientId, 
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

    messages_.erase(it); // удаление выбранного письма из очереди

    return true;
}

std::size_t MemoryQueue::releaseExpiredDeliveriesLocked(std::chrono::steady_clock::time_point now) {
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

std::size_t MemoryQueue::releaseExpiredDeliveries() {
    std::lock_guard<std::mutex> lock(mutex_);

    return releaseExpiredDeliveriesLocked(std::chrono::steady_clock::now());
}

std::size_t MemoryQueue::removeExpiredMessagesLocked(std::chrono::steady_clock::time_point now) {
    const auto oldSize = messages_.size();

    // удаление из messages_ всех записей для которых условие вернуло true
    auto newEnd = std::remove_if(
        messages_.begin(), 
        messages_.end(),
        [now](const Entry& entry) { 
            return entry.expiresAt && now >= *entry.expiresAt; 
        });
    messages_.erase(newEnd, messages_.end()
    );

    const std::size_t removed = oldSize - messages_.size();
    if (removed > 0) {
        Logger::log("Очередь " + name_ + ": по TTL удалено сообщений: " + std::to_string(removed));
    }
    
    return removed;
}

std::size_t MemoryQueue::removeExpiredMessages() {
    std::lock_guard<std::mutex> lock(mutex_);

    return removeExpiredMessagesLocked(std::chrono::steady_clock::now());
}

bool MemoryQueue::isMessageAvailable(const std::string& messageId) {
    std::lock_guard<std::mutex> lock(mutex_);

    removeExpiredMessagesLocked(std::chrono::steady_clock::now());

    return std::any_of(messages_.begin(), messages_.end(), [&](const Entry& entry) {
        return entry.item.message.id == messageId;
    });
}

// удаление пометок выдачи письма при отписке клиента от очереди 
void MemoryQueue::releaseAllDeliveriesForClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (Entry& entry : messages_) {
        if (entry.active && entry.active->clientId == clientId) {
            entry.active.reset();
        }
    }
}

// отменить одно резервирование|одну активную выдачу письма клиенту при проблеме с выдачей
void MemoryQueue::releaseActiveDelivery(const std::string& clientId, const std::string& messageId, const std::string& deliveryAttemptId) {
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

std::size_t MemoryQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return messages_.size();
}
