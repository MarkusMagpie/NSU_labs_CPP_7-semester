#include <string>

#include <common/Logger.hpp>
#include <common/YamlConfig.hpp>
#include <protocol/Envelope.hpp>
#include <zmq.hpp>

int main(int argc, char* argv[]) {
    const std::string configPath = argc > 1 ? argv[1] : "config/broker.yaml";

    // 1 - чтение yaml конфига очередей
    BrokerConfig config = BrokerConfig::fromFile(configPath);
    Logger::log("Конфиг загружен. В нем " + std::to_string(config.queues.size()) + " очередей:");
    for (const QueueConfig& queue : config.queues) {
        // логирование инфы о очередях
        std::string line = "  очередь \"" + queue.name + "\": durability=" + queue.durability
            + ", order=" + queue.order;
        if (queue.messageTtl) {
            line += ", ttl=" + std::to_string(*queue.messageTtl) + "s";
        } else {
            line += ", ttl=без";
        }

        Logger::log(line);
    }

    // 2 - ZeroMQ привязь сокета на адрес из yaml конфига 
    zmq::context_t context(1);
    zmq::socket_t router(context, zmq::socket_type::router);
    router.bind(config.bind); // сокет router привязывается к адресу в переменной bind
    Logger::log("Брокер слушает " + config.bind);

    // todo 3 - обрабатывать сообщения на порту

    // 3 - тест сериализации конверта
    Envelope probe;
    probe.type = MsgType::Ok;
    probe.queue = "smoke";
    Logger::log("Проверка конверта: " + probe.toJson());

    return 0;
}
