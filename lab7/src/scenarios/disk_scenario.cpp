#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <client/BrokerClient.hpp>
#include <protocol/Envelope.hpp>

namespace {
const std::string queue = "DiskScenarioQueue";
const std::string producerId = "123";
const std::string text = "проверка дисковой очереди";

Envelope request(BrokerClient& client, const Envelope& message) {
    client.send(message.toJson());
    return Envelope::fromJson(client.receive());
}

void expect(const Envelope& reply, MsgType expected) {
    if (reply.type != expected) {
        throw std::runtime_error("Неожиданный ответ: " + reply.toJson());
    }
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 2 || (std::string(argv[1]) != "send"
            && std::string(argv[1]) != "receive" && std::string(argv[1]) != "check")) {
            throw std::runtime_error("Использование: disk_scenario send | receive <msg_id> | check");
        }
        const std::string mode = argv[1];
        if (mode == "receive" && argc != 3) {
            throw std::runtime_error("Укажите msg_id, полученный при отправке");
        }

        BrokerClient client("tcp://127.0.0.1:5557");
        Envelope message;
        if (mode == "send") {
            // объявление клиента-производителя сообщений
            message.type = MsgType::DeclareProducer;
            message.producerQos = ProducerQos::BrokerAck;
            expect(request(client, message), MsgType::Ok);

            // отправка сообщения производителем
            message = Envelope{};
            message.type = MsgType::Produce;
            message.queue = queue;
            message.producerId = producerId;
            message.payload = text;
            const Envelope reply = request(client, message);
            expect(reply, MsgType::Ok);

            std::cout << "Сообщение сохранено. msg_id=" << reply.msgId << '\n';
        } else {
            // подписка на очередь клиента-потребителя
            message.type = MsgType::Subscribe;
            message.queue = queue;
            message.consumerQos = ConsumerQos::WithAck;
            message.ackTimeoutMs = 30000;
            expect(request(client, message), MsgType::Ok);

            // отправка сообщения на получение сообщения
            message = Envelope{};
            message.type = MsgType::Receive;
            message.queue = queue;
            const Envelope reply = request(client, message);

            expect(reply, MsgType::Deliver);
            if (reply.msgId != argv[2] 
                || reply.queue != queue 
                || reply.producerId != producerId
                || reply.payload != nlohmann::json(text) 
                || reply.deliveryAttemptId.empty()) {
                throw std::runtime_error("Получено неожиданное сообщение: " + reply.toJson());
            }

            message = Envelope{};
            message.type = MsgType::Ack;
            message.queue = queue;
            message.msgId = reply.msgId;
            message.deliveryAttemptId = reply.deliveryAttemptId;
            expect(request(client, message), MsgType::Ok);
            std::cout << "Сообщение " << reply.msgId << " получено и подтверждено\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "Ошибка сценария: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
