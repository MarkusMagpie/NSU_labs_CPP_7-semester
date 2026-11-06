#include <stdexcept>

#include <client/BrokerClient.hpp>

BrokerClient::BrokerClient(const std::string& address) : 
    context_(1), // количество внутренних I/O потоков ZeroMQ для этого контекста
    socket_(context_, zmq::socket_type::dealer) 
{
    socket_.connect(address);
}

void BrokerClient::send(const std::string& message) {
    if (!socket_.send(zmq::buffer(message), zmq::send_flags::none)) {
        throw std::runtime_error("Не удалось отправить сообщение брокеру");
    }
}

std::string BrokerClient::receive() {
    zmq::message_t reply;
    if (!socket_.recv(reply, zmq::recv_flags::none)) {
        throw std::runtime_error("Не удалось получить ответ брокера");
    }
    
    return reply.to_string();
}
