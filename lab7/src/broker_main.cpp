#include <exception>
#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
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

    // настройки подписки очереди
    struct Subscription {
        ConsumerQos qos; // уровень QoS потребителя
        std::optional<int> ackTimeoutMs;
    };

    // identity клиента -> QoS производителя (пополняется declare_producer)
    std::map<std::string, ProducerQos> producers;
    // identity клиента -> имя очереди -> настройки ее подписки
    std::map<std::string, std::map<std::string, Subscription>> subscriptions;

    // 3 - ROUTER получает идентификатор клиента + 1 фрейм с JSON
    while (true) {
        zmq::message_t identity;
        if (!router.recv(identity, zmq::recv_flags::none)) {
            continue;
        }

        bool more = identity.more(); // есть ли ещё фреймы после только что полученного
        int parts = 0; // сколько фреймов содержимого получил
        std::string body; // для хранения фрейма с json 
        while (more) {
            zmq::message_t part;
            if (!router.recv(part, zmq::recv_flags::none)) {
                break;
            }
            ++parts;
            if (parts == 1) {
                body = part.to_string();
            }
            more = part.more();
        }

        // чтобы следующее сообщение начиналось с identity надо прочитать все фреймы
        if (more || parts != 1) {
            Logger::log("Ошибка сообщения: ожидается один фрейм с JSON");
            continue;
        }

        // разбор json фрейма + составление ответа ROUTERа клиенту
        Envelope reply;
        try {
            Envelope envelope = Envelope::fromJson(body);
            Logger::log("Получен конверт: type=" + Envelope::typeToString(envelope.type) 
                + ", queue=" + envelope.queue 
                + ", payload=" + envelope.payload.dump());
            reply.type = MsgType::Ok;
            const std::string client = identity.to_string();
            if (envelope.type == MsgType::DeclareProducer) {
                if (!envelope.producerQos) {
                    throw std::runtime_error("Для declare_producer требуется producer_qos");
                }

                producers.insert_or_assign(client, *envelope.producerQos);
                reply.payload = "Производитель зарегистрирован";
            } else if (envelope.type == MsgType::Subscribe) {
                /* пример сообщения подписки на очередь: 
                {"type": "subscribe", "queue": "HighPriorityQueue", "consumer_qos": "with_ack", "ack_timeout_ms": 5000} 
                */
                if (envelope.queue.empty() || !envelope.consumerQos) {
                    throw std::runtime_error("Для subscribe требуются queue и consumer_qos");
                }
                
                // поиск в config.queues описания очереди с именем, переданным клиентом
                const auto queue = std::find_if(
                    config.queues.begin(), 
                    config.queues.end(),
                    [&envelope](const QueueConfig& item) { 
                        return item.name == envelope.queue; 
                    }
                );
                if (queue == config.queues.end()) {
                    throw std::runtime_error("Неизвестная очередь: " + envelope.queue);
                }

                // по ТЗ потребитель с подтверждением задает период ожидания подтверждения при подписке
                if (*envelope.consumerQos == ConsumerQos::WithAck
                    && (!envelope.ackTimeoutMs || *envelope.ackTimeoutMs <= 0)) {
                    throw std::runtime_error("Для with_ack QoS требуется положительный ack_timeout_ms");
                }
                // сохранение подписки
                subscriptions[client].insert_or_assign(envelope.queue,
                    Subscription{
                        *envelope.consumerQos, 
                        envelope.ackTimeoutMs
                    }
                );

                reply.queue = envelope.queue;
                reply.payload = "Подписка зарегистрирована";
            } else if (envelope.type == MsgType::Unsubscribe) {
                /* пример сообщения отписки от очереди:
                {"type": "unsubscribe", "queue": "HighPriorityQueue"}
                */
                if (envelope.queue.empty()) {
                    throw std::runtime_error("Для unsubscribe требуется queue");
                }

                // удаляю только подписку отправившего письмо клиента на указанную в письме очередь
                auto clientSubscriptions = subscriptions.find(client);
                if (clientSubscriptions == subscriptions.end()
                    || clientSubscriptions->second.erase(envelope.queue) == 0) {
                    throw std::runtime_error("Подписка на очередь не найдена: " + envelope.queue);
                }
                // если у клиента больше не осталось подписок, то удаляется его пустая запись из subscriptions
                if (clientSubscriptions->second.empty()) {
                    subscriptions.erase(clientSubscriptions);
                }

                reply.queue = envelope.queue;
                reply.payload = "Подписка отменена";
            } else {
                throw std::runtime_error("Тип сообщения пока не поддерживается: "
                    + Envelope::typeToString(envelope.type));
            }
        } catch (const std::exception& error) {
            Logger::log("Ошибка обработки конверта: " + std::string(error.what()));
            
            reply.type = MsgType::Error;
            reply.payload = error.what();
        }

        // ответ ROUTERа клиенту по его identity в 2 фреймах
        std::string response = reply.toJson();
        router.send(zmq::buffer(identity.data(), identity.size()), zmq::send_flags::sndmore);
        router.send(zmq::buffer(response), zmq::send_flags::none);
    }
}
