#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <client/BrokerClient.hpp>
#include <protocol/Envelope.hpp>

namespace {
const std::string queue = "TemperatureQueue";

Envelope request(BrokerClient& client, const Envelope& message) {
    client.send(message.toJson());
    return Envelope::fromJson(client.receive());
}

void expect(const Envelope& reply, MsgType type) {
    if (reply.type != type) {
        throw std::runtime_error("Неожиданный ответ: " + reply.toJson());
    }
}

struct Reading {
    std::int64_t timestamp; // время показания
    int temperature;
    std::string id; // msg_id который вернул брокер после отправки сообщения ему
};

void subscribe(BrokerClient& consumer) {
    Envelope message;
    message.type = MsgType::Subscribe;
    message.queue = queue;
    message.consumerQos = ConsumerQos::NoAck;

    expect(request(consumer, message), MsgType::Ok);
}

void readAll(BrokerClient& consumer, const std::string& name, const std::vector<Reading>& readings) {
    Envelope message;
    message.type = MsgType::Receive;
    message.queue = queue;

    /*
    при каждом receive брокер для NoAck + Sorted вызывает:
        queue.findNextSortedMessage(state.lastSortedPosition);
    очередь выбирает минимальное подходящее сообщение по (timestamp, sequence)
    */

    for (const auto& reading : readings) {
        const Envelope reply = request(consumer, message);
        expect(reply, MsgType::Deliver);
        
        // тут уже важно сохранение исходного порядка сообщений 
        if (reply.queue != queue 
            || reply.payload != nlohmann::json{{"timestamp", reading.timestamp}, {"temperature", reading.temperature}}) {
            throw std::runtime_error(name + ": неожиданное сообщение: " + reply.toJson());
        }

        std::cout << name << ": timestamp=" << reading.timestamp << ", температура=" << reading.temperature << ", msg_id=" << reply.msgId << '\n';
    }
    
    // 4 показание убедиться что больше сообщений нема (для конкретного потребителя)
    expect(request(consumer, message), MsgType::NoMessage);
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc > 2) throw std::runtime_error("Использование: temperature_scenario [адрес]");
        
        const std::string address = argc == 2 ? argv[1] : "tcp://127.0.0.1:5569";

        BrokerClient sensor(address), first(address), second(address);

        // датчик объявляет себя производителем
        Envelope message;
        message.type = MsgType::DeclareProducer;
        message.producerQos = ProducerQos::BrokerAck;
        expect(request(sensor, message), MsgType::Ok);

        // время с эпохи в секундах
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // показания за последние сутки не в хронологическом порядке
        std::vector<Reading> readings{{now - 23 * 3600, 15, {}},
                                      {now - 12 * 3600, 21, {}},
                                      {now - 1 * 3600, 18, {}}
                                    };

        // датчик отправляет показания (не по порядку)
        // смотри timestamp у брокера и пойми как они внутри сортируются
        for (const auto index : {2, 0, 1}) {
            message = Envelope{};

            message.type = MsgType::Produce;
            message.queue = queue;
            message.producerId = "temperature-sensor";
            message.payload = {
                {"timestamp", readings[index].timestamp}, 
                {"temperature", readings[index].temperature}
            };
            
            const Envelope reply = request(sensor, message);
            expect(reply, MsgType::Ok);
            
            if (reply.msgId.empty()) throw std::runtime_error("Брокер не вернул msg_id");
            
            readings[index].id = reply.msgId;

            std::cout << "Датчик отправил timestamp=" << readings[index].timestamp << ", температура=" << readings[index].temperature << '\n';
        }

        // подписи потребителей (MsgType::Subscribe)
        subscribe(first);
        subscribe(second);

        // оба читают все показания датчика
        readAll(first, "Потребитель 1", readings);
        readAll(second, "Потребитель 2", readings);
        
        std::cout << "сценарий завершен успешно\n";
    } catch (const std::exception& error) {
        std::cerr << "сценарий не пройден: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
