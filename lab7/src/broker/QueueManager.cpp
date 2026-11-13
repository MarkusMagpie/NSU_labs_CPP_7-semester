#include <stdexcept>
#include <limits>

#include <broker/QueueManager.hpp>
#include <broker/DiskStorage.hpp>
#include <common/Logger.hpp>

QueueManager::QueueManager(const std::vector<QueueConfig>& configs, const std::filesystem::path& dataDir) {
    /* если в yaml конфигурации есть хотя бы одна очередь с durability: disk, то 
    data/broker-state.json: { "next_message_id": 8 } 
    то есть следующее создаваемое сообщение получит в json msg_id = 8
    */
    for (const auto& config : configs) {
        if (config.durability == "disk") {
            std::filesystem::create_directories(dataDir);
            counterPath_ = dataDir / "broker-state.json";
            if (std::filesystem::exists(*counterPath_)) {
                const auto state = DiskStorage::readJson(*counterPath_);
                if (!state.contains("next_message_id") || !state["next_message_id"].is_number_unsigned()) {
                    throw std::runtime_error("Некорректный счетчик идентификаторов: " + counterPath_->string());
                }

                nextMessageId_ = state["next_message_id"].get<std::uint64_t>();
                if (nextMessageId_ == 0) {
                    throw std::runtime_error("Некорректный next_message_id");
                }
            } else {
                for (const auto& item : std::filesystem::directory_iterator(dataDir)) {
                    if (item.is_directory()) {
                        throw std::runtime_error("Отсутствует broker-state.json существующего хранилища");
                    }
                }

                DiskStorage::writeJson(*counterPath_, {
                    {"next_message_id", nextMessageId_}
                });
            }

            break;
        }
    }

    for (const QueueConfig& config : configs) {
        // 1 проверить что имя не пустое и не повторяется
        if (config.name.empty() || queues_.count(config.name) || unsupported_.count(config.name)) {
            throw std::runtime_error("Пустое или повторное имя очереди: " + config.name);
        }

        if (config.messageTtl && *config.messageTtl <= 0) {
            throw std::runtime_error("TTL очереди должен быть положительным: " + config.name);
        }

        if ((config.durability != "memory" && config.durability != "disk")
            || (config.order != "fifo" && config.order != "unordered" && config.order != "sorted")) {
            unsupported_.emplace(config.name, "поддерживаются memory/disk и fifo/unordered/sorted");
            Logger::log("Очередь " + config.name + " недоступна");
            continue;
        }

        // 2 создать объект очереди и положить в мапу
        QueueOrder order;
        if (config.order == "fifo") {
            order = QueueOrder::Fifo;
        } else if (config.order == "unordered") {
            order = QueueOrder::Unordered;
        } else {
            order = QueueOrder::Sorted;
        }
        
        std::optional<std::chrono::seconds> ttl;
        if (config.messageTtl) {
            ttl = std::chrono::seconds(*config.messageTtl);
        }

        // путь к дисковой директории
        std::optional<std::filesystem::path> diskDir;
        if (config.isDisk()) {
            const std::filesystem::path name(config.name);
            if (name.has_parent_path() || name == "." || name == ".." || name.is_absolute()) {
                throw std::runtime_error("Недопустимое имя дисковой очереди: " + config.name);
            }

            // например: data / OrdersQueue -> data/OrdersQueue
            diskDir = dataDir / name;
        }

        queues_.emplace(
            config.name, 
            std::make_unique<Queue>(config.name, order, ttl, config.sortKey, diskDir)
        );
        
        Logger::log("Создана очередь " + config.name + ", тип: " + config.durability + ", порядок: " + config.order);
    }
}

std::string QueueManager::allocateMessageId() {
    if (nextMessageId_ == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Исчерпаны идентификаторы сообщений");
    }

    if (counterPath_) {
        DiskStorage::writeJson(*counterPath_, {
            {"next_message_id", nextMessageId_ + 1}
        });
    }

    return std::to_string(nextMessageId_++);
}

Queue& QueueManager::get(const std::string& name) {
    auto queue = queues_.find(name);
    if (queue != queues_.end()) {
        return *queue->second;
    }

    auto unsupported = unsupported_.find(name);
    if (unsupported != unsupported_.end()) {
        throw std::runtime_error("Очередь " + name + " недоступна: " + unsupported->second);
    }
    
    throw std::runtime_error("Неизвестная очередь: " + name);
}

void QueueManager::releaseExpiredDeliveries() {
    // обхожу все доступные очереди и каждой говорю "проверь свои активные выдачи сообщений и освободи те у которых истек срок"
    for (const auto& [name, queue] : queues_) {
        std::size_t count = queue->releaseExpiredDeliveries();
        if (count > 0) {
            Logger::log("Очередь " + name + ": по таймауту освобождено выдач: " + std::to_string(count));
        }
    }
}

void QueueManager::removeExpiredMessages() {
    for (const auto& [name, queue] : queues_) {
        // (void)name;
        queue->removeExpiredMessages();
    }
}
