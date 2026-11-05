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
    "error"
};

constexpr int kTypeCount = static_cast<int>(sizeof(kTypeNames) / sizeof(kTypeNames[0]));
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

    envelope.payload = j.value("payload", nlohmann::json(nullptr));

    return envelope;
}
