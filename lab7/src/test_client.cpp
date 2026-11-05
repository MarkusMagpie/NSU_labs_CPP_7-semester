#include <exception>
#include <iostream>

#include <client/BrokerClient.hpp>

int main() {
    try {
        BrokerClient client("tcp://localhost:5555");
        client.send("Привет, брокер!");
        std::cout << "Сообщение принято сокетом ZeroMQ для отправки\n";
    } catch (const std::exception& error) {
        std::cerr << "Ошибка клиента: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
