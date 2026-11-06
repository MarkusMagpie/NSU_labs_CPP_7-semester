#include <exception>
#include <iostream>
#include <string_view>

#include <client/BrokerClient.hpp>
#include <protocol/Envelope.hpp>

int main(int argc, char* argv[]) {
    try {
        BrokerClient client("tcp://localhost:5555");
        if (argc > 1 && std::string_view(argv[1]) == "--invalid") {
            client.send("Это не JSON");
        } else {
            Envelope envelope;
            envelope.type = MsgType::Produce;
            envelope.queue = "HighPriorityQueue";
            envelope.payload = "Привет, брокер!";
            client.send(envelope.toJson());
        }
        std::cout << "Сообщение принято сокетом ZeroMQ для отправки\n";

        Envelope reply = Envelope::fromJson(client.receive());
        std::cout << "Ответ брокера: type=" << Envelope::typeToString(reply.type)
                  << ", payload=" << reply.payload.dump() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Ошибка клиента: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
