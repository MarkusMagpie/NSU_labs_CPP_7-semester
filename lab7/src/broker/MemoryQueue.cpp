#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

#include <broker/MemoryQueue.hpp>
#include <common/Logger.hpp>

MemoryQueue::MemoryQueue(std::string name, QueueOrder order,
    std::optional<std::chrono::seconds> messageTtl)
    : name_(std::move(name)), order_(order), messageTtl_(messageTtl) 
{
    if (messageTtl_ && *messageTtl_ <= std::chrono::seconds::zero()) {
        throw std::invalid_argument("TTL очереди должен быть положительным");
    }
}

const std::string& MemoryQueue::name() const {
    return name_;
}

QueueOrder MemoryQueue::order() const {
    return order_;
}

/*добавление сообщения в очередь*/
void MemoryQueue::push(Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (nextSequence_ == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Исчерпаны порядковые номера очереди: " + name_);
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
            expiresAt
        }
    );

    ++nextSequence_;
}

// (для потребителя с NoAck QoS) ищет первое письмо с номером больше lastSequence
std::optional<SequencedMessage> MemoryQueue::findNextMessageAfter(std::uint64_t lastSequence) {
    std::lock_guard<std::mutex> lock(mutex_);

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

// (для потребителя с WithAck QoS) выбирает сообщение из очереди и закрепляет его за указанным потребителем по clientId
std::optional<Delivery> MemoryQueue::reserveNextMessageForAck(const std::string& clientId, std::chrono::milliseconds ackTimeout) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (ackTimeout <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Таймаут подтверждения должен быть положительным");
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
        std::to_string(nextDeliveryAttemptId_)
    };
    it->active = ActiveDelivery{
        clientId, 
        delivery.deliveryAttemptId,
        std::chrono::steady_clock::now() + ackTimeout // дедлайн = текущий момент + длительность ожидания подтверждения получения сообщения потребителем
    };

    // 3 - теперь сообщение закреплено за потребителем и выдано ему  
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
