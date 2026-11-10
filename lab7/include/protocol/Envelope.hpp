#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

// тип сообщения в брокере
enum class MsgType {
    DeclareProducer,    // производитель/producer -> брокер: объявляет QoS
    Produce,            // производитель -> брокер: новое сообщение

    Subscribe,          // потребитель/consumer -> брокер: подписка на очередь в брокере
    Unsubscribe,        // потребитель -> брокер: отписка от очереди
    Ack,                // потребитель -> брокер: подтверждение обработки
    
    Deliver,            // брокер -> потребитель: доставка сообщения
    Notify,             // брокер -> производитель: сообщение обработано потребителем
    Ok,                 // брокер -> потребитель: успех
    Error,              // брокер -> потребитель: ошибка
    Receive,            // потребитель -> брокер: запрос следующего сообщения
    NoMessage           // брокер -> потребитель: новых сообщений пока нет
};

enum class ProducerQos { 
    None, 
    BrokerAck, 
    ConsumerAck 
};

enum class ConsumerQos { 
    NoAck, 
    WithAck, 
    Recovery 
};

// единый конверт для сообщений (он сериализуется в json)
struct Envelope {
    /*
    При подключении производитель объявляет, используя служебные сообщения, требуемый уровень QoS.
    Потребители сообщений явно подписываются на очереди и отписываются от очередей, посылая соответствующие служебные сообщения брокеру.
    Брокер ожидает служебного сообщения подтверждения обработки от потребителя
    */
    MsgType type = MsgType::Produce;

    /*Приложение брокера должно поддерживать множество одновременно активных сетевых клиентов (производителей/потребителей)*/
    std::string clientId;
    /*Производители и потребители явно отправляют сообщения в / получают сообщения из какой-либо очереди с указанием её имени.*/
    std::string queue; // имя очереди
    std::string msgId; // идентификатор сообщения
    std::string deliveryAttemptId; // идентификатор попытки выдачи для WithAck
    std::string producerId; // идентификатор производителя
    std::int64_t producerSeq = 0; // уникальный номер сообщения у производителя

    std::optional<std::int64_t> sortKey; // ключ сортировки (для order == sorted)
    /* 
    Время жизни сообщений ... в секундах
    Сообщения удаляются с диска и из памяти по истечении отведенного времени жизни
    */
    std::int64_t timestamp = 0; // время создания, мс
    std::optional<int> ackTimeoutMs; // таймаут подтверждения (для consumer withack)
    
    std::optional<ProducerQos> producerQos; // для declare_producer
    std::optional<ConsumerQos> consumerQos; // для subscribe

    /*Передача данных между клиентами и брокером должна осуществляться в выбранном переносимом формате сообщений - я выбрал json формат*/
    nlohmann::json payload = nullptr;

    std::string toJson() const;
    static Envelope fromJson(const std::string& text);

    static std::string typeToString(MsgType type);
    static MsgType typeFromString(const std::string& text);
};
