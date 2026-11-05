#pragma once

#include <string>

#include <zmq.hpp>

// подключение одного клиента к router сокету брокера
class BrokerClient {
public:
    explicit BrokerClient(const std::string& address);

    // отправить брокеру одну часть сообщения
    void send(const std::string& message);

private:
    zmq::context_t context_; // объект внутри которого создаются сокеты
    zmq::socket_t socket_;
};
