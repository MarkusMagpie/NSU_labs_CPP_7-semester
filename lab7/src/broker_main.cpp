#include <exception>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <broker/QueueManager.hpp>
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

    QueueManager queues(config.queues);
    std::uint64_t nextMessageId = 1;

    // 2 - ZeroMQ привязь сокета на адрес из yaml конфига 
    zmq::context_t context(1);
    zmq::socket_t router(context, zmq::socket_type::router);
    router.bind(config.bind); // сокет router привязывается к адресу в переменной bind
    Logger::log("Брокер слушает " + config.bind);

    // настройки подписки очереди
    struct Subscription {
        ConsumerQos qos; // QoS потребителя
        std::optional<int> ackTimeoutMs; // таймаут подтверждения получения сообщения 

        // fifo/unordered - номер последнего выданного сообщения
        std::uint64_t lastSequence = 0;
        // sorted - последний key и sequence для очереди с sorted порядком обработки сообщений; nullopt - чтение не начиналось
        std::optional<SortedReadPosition> lastSortedPosition;
    };

    // id клиента -> QoS производителя (пополняется declare_producer)
    std::map<std::string, ProducerQos> producers;
    // id клиента -> имя очереди -> настройки подписки очереди 
    std::map<std::string, std::map<std::string, Subscription>> subscriptions;

    // 3 - ROUTER получает идентификатор клиента + 1 фрейм с JSON
    while (true) {
        // удалить собщения у которых истек ттл
        queues.removeExpiredMessages();
        // у оставшихся сообщений освободить истекшие попытки подтверждения выдачи сообщений от WithAck потребителей
        queues.releaseExpiredDeliveries();

        // объект для ожидания событи: слежка за сокетом router и событием ZMQ_POLLIN - возможность читать входящие данные
        zmq::pollitem_t items[] = {
            {router.handle(), 0, ZMQ_POLLIN, 0}
        };
        // брокер ждет пока на сокете появятся входящие данные || пока закончится период ожидания
        zmq::poll(items, 1, std::chrono::milliseconds(50));
        // проверка есть ли входящее сообщение (revents содержит события которые произошли)
        if (!(items[0].revents & ZMQ_POLLIN)) {
            continue;
        }

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
        bool sendReply = true;
        std::uint64_t* cursorToAdvance = nullptr;
        std::uint64_t deliveredSequence = 0;
        // какую именно позицию подписки потом обновить
        std::optional<SortedReadPosition>* sortedCursorToAdvance = nullptr;
        // какое значение записать в sortedCursorToAdvance
        std::optional<SortedReadPosition> deliveredSortedPosition;
        
        /* указатель на очередь в которой брокер в текущей итерации обработал запрос receive от потребителя WithAck */
        MemoryQueue* claimedQueue = nullptr;
        MemoryQueue* deliveryQueue = nullptr;
        const std::string client_id = identity.to_string();
        try {
            Envelope envelope = Envelope::fromJson(body);
            Logger::log("Получен конверт: type=" + Envelope::typeToString(envelope.type) 
                + ", queue=" + envelope.queue 
                + ", payload=" + envelope.payload.dump());
            reply.type = MsgType::Ok;
            if (envelope.type == MsgType::DeclareProducer) {
                if (!envelope.producerQos) {
                    throw std::runtime_error("Для declare_producer требуется producer_qos");
                }

                producers.insert_or_assign(client_id, *envelope.producerQos);
                reply.payload = "Производитель зарегистрирован";
            } else if (envelope.type == MsgType::Subscribe) {
                /* пример сообщения подписки на очередь: 
                {"type": "subscribe", "queue": "HighPriorityQueue", "consumer_qos": "with_ack", "ack_timeout_ms": 5000} 
                */
                if (envelope.queue.empty() || !envelope.consumerQos) {
                    throw std::runtime_error("Для subscribe требуются queue и consumer_qos");
                }
                
                // подписка возможна только на очередь поддерживаемую на текущем этапе разработки
                queues.get(envelope.queue);

                // по ТЗ потребитель с подтверждением задает период ожидания подтверждения при подписке
                if (*envelope.consumerQos == ConsumerQos::WithAck
                    && (!envelope.ackTimeoutMs || *envelope.ackTimeoutMs <= 0)) {
                    throw std::runtime_error("Для with_ack QoS требуется положительный ack_timeout_ms");
                }

                // повторная подписка отменяет старые выдачи этого клиента в очереди
                queues.get(envelope.queue).releaseAllDeliveriesForClient(client_id);
                
                // сохранение подписки
                subscriptions[client_id].insert_or_assign(envelope.queue,
                    Subscription{
                        *envelope.consumerQos, 
                        envelope.ackTimeoutMs,
                        0,
                        std::nullopt
                    }
                );

                reply.queue = envelope.queue;
                reply.payload = "Подписка зарегистрирована";
            } else if (envelope.type == MsgType::Receive) {
                /* пример сообщения потребителя брокеру для получения письма
                {"type": "receive", "queue": "MemoryFifoQueue"}
                */
                if (envelope.queue.empty()) {
                    throw std::runtime_error("Для receive требуется queue");
                }

                // проверка имени очереди и подписки у клиента
                auto clientSubscriptions = subscriptions.find(client_id);
                if (clientSubscriptions == subscriptions.end()) {
                    throw std::runtime_error("Клиент не подписан на очередь: " + envelope.queue);
                }
                auto subscription = clientSubscriptions->second.find(envelope.queue);
                if (subscription == clientSubscriptions->second.end()) {
                    throw std::runtime_error("Клиент не подписан на очередь: " + envelope.queue);
                }

                Subscription& state = subscription->second;
                MemoryQueue& queue = queues.get(envelope.queue);
                reply.queue = envelope.queue;

                // получение письма варьируется от QoS потребителя
                if (state.qos == ConsumerQos::NoAck) {
                    if (queue.order() == QueueOrder::Sorted) {
                        // выбор следующего письма по (key, sequence)
                        auto result = queue.findNextSortedMessage(state.lastSortedPosition);
                        if (result) {
                            deliveryQueue = &queue;
                            reply.type = MsgType::Deliver;
                            reply.msgId = result->item.message.id;
                            reply.producerId = result->item.message.producerId;
                            reply.payload = result->item.message.payload;

                            sortedCursorToAdvance = &state.lastSortedPosition;
                            deliveredSortedPosition = std::move(result->position);
                        } else {
                            reply.type = MsgType::NoMessage;
                        }
                    } else {
                        // fifo и unordered читаются по sequence, не удаляя письмо
                        std::optional<SequencedMessage> message = queue.findNextMessageAfter(state.lastSequence);
                        if (message) {
                            deliveryQueue = &queue;
                            reply.type = MsgType::Deliver;
                            reply.msgId = message->message.id;
                            reply.producerId = message->message.producerId;
                            reply.payload = message->message.payload;

                            cursorToAdvance = &state.lastSequence;
                            deliveredSequence = message->sequence;
                        } else {
                            reply.type = MsgType::NoMessage;
                        }
                    }
                } else if (state.qos == ConsumerQos::WithAck) {
                    // найти следующее сообщение и зарезервировать его для выдачи с подтверждением указанному потребителю (по его client_id)
                    std::optional<Delivery> delivery = queue.reserveNextMessageForAck(client_id, std::chrono::milliseconds(*state.ackTimeoutMs));
                    if (delivery) {
                        /*
                        пример ответа: 
                        {
                        "type": "deliver", 
                        "queue": "MemoryFifoQueue", 
                        "msg_id": "10", 
                        "delivery_attempt_id": "5", 
                        "producer_id": "producer-1", 
                        "payload": "ляляля"
                        }
                        */
                        
                        claimedQueue = &queue;
                        deliveryQueue = &queue;

                        reply.type = MsgType::Deliver;
                        reply.msgId = delivery->message.id;
                        reply.deliveryAttemptId = delivery->deliveryAttemptId;
                        reply.producerId = delivery->message.producerId;
                        reply.payload = delivery->message.payload;
                    } else {
                        reply.type = MsgType::NoMessage;
                    }
                } else {
                    throw std::runtime_error("Получение с recovery пока не поддерживается");
                }
            } else if (envelope.type == MsgType::Ack) {
                if (envelope.queue.empty() || envelope.msgId.empty() || envelope.deliveryAttemptId.empty()) {
                    throw std::runtime_error("Для ack требуются queue, msg_id и delivery_attempt_id");
                }

                MemoryQueue& queue = queues.get(envelope.queue);
                
                /*
                в очереди ищется письмо в котором: 
                    id письма совпал
                    есть активная выдача
                    identity клиента совпал
                    id попытки выдачи совпал
                пример: "клиент 1 подтверждает msg_id=10, attempt=5" -> из очереди в брокере удаляется соответсвующее письмо 
                */
                if (!queue.confirmDeliveryAndRemoveMessage(client_id, envelope.msgId, envelope.deliveryAttemptId)) {
                    // просроченное, повторное или чужое подтверждение ничего не меняет
                    Logger::log("Подтверждение проигнорировано: выдача сообщения не найдена или истекло время подтверждения");
                    continue;
                }
                reply.queue = envelope.queue;
                reply.msgId = envelope.msgId;
                reply.payload = "обработка подтверждена";

                Logger::log("Сообщение #" + envelope.msgId + " подтверждено и удалено из очереди " + envelope.queue);
            } else if (envelope.type == MsgType::Unsubscribe) {
                /* пример сообщения отписки от очереди:
                {"type": "unsubscribe", "queue": "HighPriorityQueue"}
                */
                if (envelope.queue.empty()) {
                    throw std::runtime_error("Для unsubscribe требуется queue");
                }

                // удаляю только подписку отправившего письмо клиента на указанную в письме очередь
                auto clientSubscriptions = subscriptions.find(client_id);
                if (clientSubscriptions == subscriptions.end()
                    || clientSubscriptions->second.erase(envelope.queue) == 0) {
                    throw std::runtime_error("Подписка на очередь не найдена: " + envelope.queue);
                }
                // если у клиента больше не осталось подписок, то удаляется его пустая запись из subscriptions
                if (clientSubscriptions->second.empty()) {
                    subscriptions.erase(clientSubscriptions);
                }

                queues.get(envelope.queue).releaseAllDeliveriesForClient(client_id);

                reply.queue = envelope.queue;
                reply.payload = "Подписка отменена";
            } else if (envelope.type == MsgType::Produce) {
                /* пример сообщения о новом сообщении от производителя брокеру:
                {"type":"produce","queue":"MemoryFifoQueue","producer_id":"producer-1","payload":"привет"}
                */
                const auto producer = producers.find(client_id);
                if (producer == producers.end()) {
                    throw std::runtime_error("Сначала необходимо объявить производителя");
                }

                // для qos без подтверждения не посылаю ответ на produce, даже при ошибке
                sendReply = producer->second != ProducerQos::None;
                if (producer->second == ProducerQos::ConsumerAck) {
                    throw std::runtime_error("QoS consumer_ack пока не поддерживается для produce");
                }
                if (envelope.queue.empty() || envelope.producerId.empty()) {
                    throw std::runtime_error("Для produce требуются queue и producer_id");
                }

                // найти очередь в брокере по ее названию в письме
                MemoryQueue& queue = queues.get(envelope.queue);

                Message message{
                    std::to_string(nextMessageId++), 
                    envelope.producerId, 
                    envelope.payload
                };
                reply.msgId = message.id;
                reply.queue = envelope.queue;
                queue.push(std::move(message)); // запись письма в конец очереди
                Logger::log("Сообщение #" + reply.msgId + " добавлено в очередь " + envelope.queue);
                reply.payload = "Сообщение добавлено в очередь";
            } else {
                throw std::runtime_error("Тип сообщения пока не поддерживается: "
                    + Envelope::typeToString(envelope.type));
            }
        } catch (const std::exception& error) {
            if (claimedQueue) {
                claimedQueue->releaseActiveDelivery(client_id, reply.msgId, reply.deliveryAttemptId);
                claimedQueue = nullptr;
            }
            deliveryQueue = nullptr;
            cursorToAdvance = nullptr;
            sortedCursorToAdvance = nullptr;
            Logger::log("Ошибка обработки конверта: " + std::string(error.what()));
            
            reply.type = MsgType::Error;
            reply.payload = error.what();
        }

        if (!sendReply) {
            continue;
        }

        // при подготовке ответа ttl мог закончиться после выбора сообщения
        if (deliveryQueue && !deliveryQueue->isMessageAvailable(reply.msgId)) {
            std::string queueName = reply.queue;
            reply = Envelope{};
            reply.type = MsgType::NoMessage;
            reply.queue = queueName;

            cursorToAdvance = nullptr;
            sortedCursorToAdvance = nullptr;
            claimedQueue = nullptr;
        }

        // ответ ROUTERа клиенту по его identity в 2 фреймах
        std::string response = reply.toJson();
        const auto identitySent = router.send(zmq::buffer(identity.data(), identity.size()), zmq::send_flags::sndmore);
        if (!identitySent) {
            Logger::log("Не удалось отправить адрес ответа клиенту");
            if (claimedQueue) claimedQueue->releaseActiveDelivery(client_id, reply.msgId, reply.deliveryAttemptId);
            continue;
        }
        const auto responseSent = router.send(zmq::buffer(response), zmq::send_flags::none);
        if (!responseSent) {
            Logger::log("Не удалось отправить ответ клиенту");
            if (claimedQueue) claimedQueue->releaseActiveDelivery(client_id, reply.msgId, reply.deliveryAttemptId);
            continue;
        }
        if (cursorToAdvance) {
            *cursorToAdvance = deliveredSequence;
        }
        if (sortedCursorToAdvance) {
            *sortedCursorToAdvance = std::move(deliveredSortedPosition);
        }
    }
}
