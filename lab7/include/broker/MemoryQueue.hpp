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
    Unordered,
    Sorted
};

// id сообщения не меняется при удалении других сообщений
struct SequencedMessage {
    std::uint64_t sequence;
    Message message;
};

// последнее письмо полученное потребителем в очереди с sorted order
struct SortedReadPosition {
    nlohmann::json sortValue; // = key
    std::uint64_t sequence;
};

// результат поиска письма для потребителя с sorted order
struct SortedReadResult {
    SequencedMessage item; // копия найденного письма
    SortedReadPosition position;
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
    std::optional<SortedReadPosition> sortedPosition; // для очереди с sorted order: позиция письма переданного потребителю
};

class MemoryQueue {
private:
    // запись в очереди   
    struct Entry {
        SequencedMessage item; // сообщение + его порядковый номер в очереди
        // active = nullopt -> письмо доступно для выдачи потребителю с WithAck
        // active заполнено -> письмо выдано и ожидает подтверждения потребителем с withack
        std::optional<ActiveDelivery> active; // информация о выдаче
        std::optional<std::chrono::steady_clock::time_point> expiresAt; // момент истечения TTL = сообщение перестанет жить в очереди
        std::optional<nlohmann::json> sortValue; // целое знаковое или беззнаковое значение sortKey (key)
    };
    
    const std::string name_;
    const QueueOrder order_;
    const std::optional<std::chrono::seconds> messageTtl_;
    const std::string sortKey_; // имя числового поля внутри payload

    std::deque<Entry> messages_;
    mutable std::mutex mutex_; // messages_ def

    std::uint64_t nextSequence_ = 1; // номер добавляемого сообщения
    std::uint64_t nextDeliveryAttemptId_ = 1; // номер следующей попытки выдачи в очереди

    // -1: left < right; 0: равны; 1: left > right
    static int compareIntegerSortValues(
        const nlohmann::json& left, 
        const nlohmann::json& right
    );
    // сначала сравнить значения ключа, при их равенстве порядковые номера lastSequence
    // "должна ли запись left идти раньше записи right в сортированном порядке"
    static bool isEntryBeforeBySortKey(const Entry& left, const Entry& right);

    // !!! вызывать только под mutex_
    // возвращает число освобожденных выдач
    std::size_t releaseExpiredDeliveriesLocked(std::chrono::steady_clock::time_point now);
    // удаляет сообщения у которых истек ttl
    std::size_t removeExpiredMessagesLocked(std::chrono::steady_clock::time_point now);
public:
    MemoryQueue(std::string name, QueueOrder order,
        std::optional<std::chrono::seconds> messageTtl = std::nullopt,
        std::string sortKey = {});

    const std::string& name() const;
    QueueOrder order() const;

    void push(Message message);

    // 1 доступное сообщение с номером больше lastSequence; 0 означает начало
    std::optional<SequencedMessage> findNextMessageAfter(std::uint64_t lastSequence);

    // найти минимальную пару (key, sequence) после позиции
    std::optional<SortedReadResult> findNextSortedMessage(const std::optional<SortedReadPosition>& lastPosition);

    // выбрать и закрепить сообщение за одним потребителем с WithAck QoS
    std::optional<Delivery> reserveNextMessageForAck(
        const std::string& clientId, 
        std::chrono::milliseconds ackTimeout,
        const std::optional<SortedReadPosition>& lastSortedPosition = std::nullopt);
    // удалить сообщение только при совпадении клиента и активной попытки
    bool confirmDeliveryAndRemoveMessage(const std::string& clientId, const std::string& messageId, const std::string& deliveryAttemptId);
    // отменить выдачи при отписке или изменении подписки
    void releaseAllDeliveriesForClient(const std::string& clientId);
    // отменить конкретную выдачу если ответ не удалось отправить сокету потребителя
    void releaseActiveDelivery(const std::string& clientId, const std::string& messageId,
        const std::string& deliveryAttemptId);

    // освободить выдачи для которых истек срок ожидания подтверждения
    std::size_t releaseExpiredDeliveries();

    // удалить сообщения с истекшим TTL; вернуть количество удаленных
    std::size_t removeExpiredMessages();
    // проверка письма перед отправкой ответа клиенту
    bool isMessageAvailable(const std::string& messageId);

    std::size_t size() const;
};
