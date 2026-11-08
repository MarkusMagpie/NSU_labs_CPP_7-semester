#include <stdexcept>

#include <protocol/Envelope.hpp>

namespace {
constexpr const char* kTypeNames[] = {
    "declare_producer",
    "produce",
    "subscribe",
    "unsubscribe",
    "ack",
    "deliver",
    "notify",
    "ok",
    "error",
    "receive",
    "no_message"
};

constexpr int kTypeCount = static_cast<int>(sizeof(kTypeNames) / sizeof(kTypeNames[0]));

const char* producerQosName(ProducerQos qos) {
    switch (qos) {
        case ProducerQos::None: return "none";
        case ProducerQos::BrokerAck: return "broker_ack";
        case ProducerQos::ConsumerAck: return "consumer_ack";
    }

    throw std::runtime_error("protocol: неизвестный QoS производителя");
}

ProducerQos parseProducerQos(const std::string& value) {
    if (value == "none") {
        return ProducerQos::None;
    }
    if (value == "broker_ack") return ProducerQos::BrokerAck;
    if (value == "consumer_ack") return ProducerQos::ConsumerAck;
    
    throw std::runtime_error("protocol: неизвестный QoS производителя: " + value);
}

const char* consumerQosName(ConsumerQos qos) {
    switch (qos) {
        case ConsumerQos::NoAck: return "no_ack";
        case ConsumerQos::WithAck: return "with_ack";
        case ConsumerQos::Recovery: return "recovery";
    }

    throw std::runtime_error("protocol: неизвестный QoS потребителя");
}

ConsumerQos parseConsumerQos(const std::string& value) {
    if (value == "no_ack") return ConsumerQos::NoAck;
    if (value == "with_ack") return ConsumerQos::WithAck;
    if (value == "recovery") return ConsumerQos::Recovery;

    throw std::runtime_error("protocol: неизвестный QoS потребителя: " + value);
}

} // namespace

std::string Envelope::typeToString(MsgType type) {
    return kTypeNames[static_cast<int>(type)];
}

MsgType Envelope::typeFromString(const std::string& text) {
    for (int i = 0; i < kTypeCount; ++i) {
        if (text == kTypeNames[i]) {
            return static_cast<MsgType>(i);
        }
    }

    throw std::runtime_error("protocol: неизвестный тип сообщения '" + text + "'");
}

std::string Envelope::toJson() const {
    nlohmann::json j;

    j["type"] = typeToString(type);

    if (!clientId.empty()) j["client_id"] = clientId;
    if (!queue.empty()) j["queue"] = queue;
    if (!msgId.empty()) j["msg_id"] = msgId;
    if (!producerId.empty()) j["producer_id"] = producerId;

    j["producer_seq"] = producerSeq;
    j["timestamp"] = timestamp;

    if (sortKey) j["sort_key"] = *sortKey;
    if (ackTimeoutMs) j["ack_timeout_ms"] = *ackTimeoutMs;
    if (producerQos) j["producer_qos"] = producerQosName(*producerQos);
    if (consumerQos) j["consumer_qos"] = consumerQosName(*consumerQos);

    j["payload"] = payload;

    return j.dump();
}

Envelope Envelope::fromJson(const std::string& text) {
    nlohmann::json j = nlohmann::json::parse(text);

    Envelope envelope;
    envelope.type = typeFromString(j.at("type").get<std::string>());

    envelope.clientId = j.value("client_id", std::string{});
    envelope.queue = j.value("queue", std::string{});
    envelope.msgId = j.value("msg_id", std::string{});
    envelope.producerId = j.value("producer_id", std::string{});
    envelope.producerSeq = j.value("producer_seq", std::int64_t{0});

    envelope.timestamp = j.value("timestamp", std::int64_t{0});

    if (j.contains("sort_key") && !j["sort_key"].is_null()) {
        envelope.sortKey = j["sort_key"].get<std::int64_t>();
    }
    if (j.contains("ack_timeout_ms") && !j["ack_timeout_ms"].is_null()) {
        envelope.ackTimeoutMs = j["ack_timeout_ms"].get<int>();
    }

    if (j.contains("producer_qos")) {
        envelope.producerQos = parseProducerQos(j.at("producer_qos").get<std::string>());
    }
    if (j.contains("consumer_qos")) {
        envelope.consumerQos = parseConsumerQos(j.at("consumer_qos").get<std::string>());
    }

    envelope.payload = j.value("payload", nlohmann::json(nullptr));

    return envelope;
}
