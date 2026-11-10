#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <string>

#include <broker/Message.hpp>

enum class QueueOrder {
    Fifo,
    Unordered
};

// id сообщения не меняется при удалении других сообщений
struct SequencedMessage {
    std::uint64_t sequence;
    Message message;
};

// что очередь помнит об активной выдаче сообщения
struct ActiveDelivery {
    std::string clientId; // ROUTER identity потребителя = кому брокер выдал письмо
    std::string deliveryAttemptId; // номер попытки выдачи
    std::chrono::steady_clock::time_point deadline; // до какого момента брокер ожидает ack от потребителя
};

// результат reserveNextMessageForAck() для ответа брокеру
struct Delivery {
    Message message;
    std::string deliveryAttemptId;
};

class MemoryQueue {
private:
    // запись в очереди   
    struct Entry {
        SequencedMessage item; // сообщение + его порядковый номер в очереди
        // active = nullopt -> письмо доступно для выдачи потребителю с WithAck
        // active заполнено -> письмо выдано и ожидает подтверждения потребителем с withack
        std::optional<ActiveDelivery> active; // информация о выдаче
    };
    
    const std::string name_;
    const QueueOrder order_;

    std::deque<Entry> messages_;
    mutable std::mutex mutex_; // messages_ def

    std::uint64_t nextSequence_ = 1; // номер добавляемого сообщения
    std::uint64_t nextDeliveryAttemptId_ = 1; // номер следующей попытки выдачи в очереди

    // !!! вызывать только под mutex_
    // возвращает число освобожденных выдач
    std::size_t releaseExpiredDeliveriesLocked(std::chrono::steady_clock::time_point now);
public:
    MemoryQueue(std::string name, QueueOrder order);

    const std::string& name() const;
    QueueOrder order() const;

    void push(Message message);

    // 1 доступное сообщение с номером больше lastSequence; 0 означает начало
    std::optional<SequencedMessage> findNextMessageAfter(std::uint64_t lastSequence) const;

    // выбрать и закрепить сообщение за одним потребителем WithAck
    std::optional<Delivery> reserveNextMessageForAck(const std::string& clientId, std::chrono::milliseconds ackTimeout);
    // удалить сообщение только при совпадении клиента и активной попытки
    bool confirmDeliveryAndRemoveMessage(const std::string& clientId, const std::string& messageId,
        const std::string& deliveryAttemptId);
    // отменить выдачи при отписке или изменении подписки
    void releaseAllDeliveriesForClient(const std::string& clientId);
    // отменить конкретную выдачу если ответ не удалось отправить сокету потребителя
    void releaseActiveDelivery(const std::string& clientId, const std::string& messageId,
        const std::string& deliveryAttemptId);

    // освободить выдачи для которых истек срок ожидания подтверждения
    std::size_t releaseExpiredDeliveries();

    std::size_t size() const;
};
