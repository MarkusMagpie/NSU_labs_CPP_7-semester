#include <stdexcept>

#include <broker/QueueManager.hpp>
#include <common/Logger.hpp>

QueueManager::QueueManager(const std::vector<QueueConfig>& configs) {
    for (const QueueConfig& config : configs) {
        // 1 проверить что имя не пустое и не повторяется
        if (config.name.empty() || queues_.count(config.name) || unsupported_.count(config.name)) {
            throw std::runtime_error("Пустое или повторное имя очереди: " + config.name);
        }

        if (config.messageTtl && *config.messageTtl <= 0) {
            throw std::runtime_error("TTL очереди должен быть положительным: " + config.name);
        }

        if (config.durability != "memory" || (config.order != "fifo" && config.order != "unordered")) {
            unsupported_.emplace(config.name, "поддерживаются только memory + fifo/unordered");
            Logger::log("Очередь " + config.name + " пока недоступна: поддерживаются только memory + fifo/unordered");
            continue;
        }

        // 2 создать объект очереди и положить в мапу
        QueueOrder order = config.order == "fifo" ? QueueOrder::Fifo : QueueOrder::Unordered;
        
        std::optional<std::chrono::seconds> ttl;
        if (config.messageTtl) {
            ttl = std::chrono::seconds(*config.messageTtl);
        }

        queues_.emplace(
            config.name, 
            std::make_unique<MemoryQueue>(config.name, order, ttl)
        );
        
        Logger::log("Создана очередь в памяти: " + config.name + ", порядок: " + config.order);
    }
}

MemoryQueue& QueueManager::get(const std::string& name) {
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
