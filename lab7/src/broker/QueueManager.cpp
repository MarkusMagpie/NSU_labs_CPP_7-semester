#include <stdexcept>

#include <broker/QueueManager.hpp>
#include <common/Logger.hpp>

QueueManager::QueueManager(const std::vector<QueueConfig>& configs) {
    for (const QueueConfig& config : configs) {
        // 1 проверить что имя не пустое и не повторяется
        if (config.name.empty() || queues_.count(config.name) || unsupported_.count(config.name)) {
            throw std::runtime_error("Пустое или повторное имя очереди: " + config.name);
        }

        if (config.durability != "memory" || config.order != "fifo" || config.messageTtl) {
            unsupported_.emplace(config.name, "поддерживаются только memory + fifo без TTL");
            Logger::log("Очередь " + config.name + " пока недоступна: поддерживаются только memory + fifo без TTL");
            continue;
        }

        // 2 создать объект очереди и положить в мапу
        queues_.emplace(config.name, std::make_unique<FifoMemoryQueue>(config.name));
        Logger::log("Создана очередь в памяти: " + config.name);
    }
}

FifoMemoryQueue& QueueManager::get(const std::string& name) {
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
