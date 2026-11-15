#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <client/BrokerClient.hpp>
#include <protocol/Envelope.hpp>

namespace {

const std::string queue = "MemoryFifoQueue";

Envelope request(BrokerClient& client, const Envelope& message) {
    client.send(message.toJson());
    return Envelope::fromJson(client.receive());
}

void expect(const Envelope& reply, MsgType expected) {
    if (reply.type != expected) {
        throw std::runtime_error("Неожиданный ответ брокера: " + reply.toJson());
    }
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const std::string mode = argv[1];
        BrokerClient client("tcp://127.0.0.1:5555");
        Envelope message;

        if (mode == "producer") {
            // зарегистрировать производителя с подтверждением приема брокером
            message.type = MsgType::DeclareProducer;
            message.producerQos = ProducerQos::BrokerAck;
            // expect(request(client, message), MsgType::Ok);

            const Envelope reply = request(client, message);
            expect(reply, MsgType::Ok);
            std::cout << reply.payload.get<std::string>() << '\n';

            // отправить письмо в очередь и дождаться ответа брокера
            message = Envelope{};
            message.type = MsgType::Produce;
            message.queue = queue;
            message.producerId = "producer-1";
            message.payload = "Привет";

            const Envelope reply2 = request(client, message);
            expect(reply2, MsgType::Ok);
            std::cout << reply2.payload.get<std::string>() << '\n';

            std::cout << "Брокер принял сообщение, msg_id=" << reply2.msgId << '\n';
        } else {
            // подписаться на очередь без подтверждения получения
            message.type = MsgType::Subscribe;
            message.queue = queue;
            message.consumerQos = ConsumerQos::NoAck;
            // expect(request(client, message), MsgType::Ok);

            const Envelope reply = request(client, message);
            expect(reply, MsgType::Ok);
            std::cout << reply.payload.get<std::string>() << '\n';

            // запросить следующее письмо
            message = Envelope{};
            message.type = MsgType::Receive;
            message.queue = queue;
            const Envelope reply2 = request(client, message);
            if (reply2.type == MsgType::NoMessage) {
                std::cout << "В очереди нет доступных сообщений\n";
                return 0;
            }
            expect(reply2, MsgType::Deliver);
            std::cout << reply2.payload.get<std::string>() << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
