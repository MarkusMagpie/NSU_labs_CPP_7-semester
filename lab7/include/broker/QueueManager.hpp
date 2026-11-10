#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <broker/MemoryQueue.hpp>
#include <common/YamlConfig.hpp>

// cоздает очереди по конфигурации
class QueueManager {
private:
    std::map<std::string, std::unique_ptr<MemoryQueue>> queues_;
    std::map<std::string, std::string> unsupported_;
public:
    QueueManager(const std::vector<QueueConfig>& configs);

    MemoryQueue& get(const std::string& name);

    // проверить таймауты активных выдач сообщений во всех очередях
    void releaseExpiredDeliveries();
};
