#pragma once

#include <map>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <broker/Queue.hpp>
#include <common/YamlConfig.hpp>

// cоздает очереди по конфигурации
class QueueManager {
private:
    std::map<std::string, std::unique_ptr<Queue>> queues_;
    std::map<std::string, std::string> unsupported_;

    std::optional<std::filesystem::path> counterPath_;
    std::uint64_t nextMessageId_ = 1;
public:
    QueueManager(const std::vector<QueueConfig>& configs, const std::filesystem::path& dataDir);

    std::string allocateMessageId();

    Queue& get(const std::string& name);

    // проверить таймауты активных выдач сообщений во всех очередях
    void releaseExpiredDeliveries();

    // удалить сообщения с истекшим TTL во всех очередях
    void removeExpiredMessages();
};
