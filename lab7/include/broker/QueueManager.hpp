#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <broker/FifoMemoryQueue.hpp>
#include <common/YamlConfig.hpp>

// cоздает очереди по конфигурации
class QueueManager {
private:
    std::map<std::string, std::unique_ptr<FifoMemoryQueue>> queues_;
    std::map<std::string, std::string> unsupported_;
public:
    QueueManager(const std::vector<QueueConfig>& configs);

    FifoMemoryQueue& get(const std::string& name);
};
