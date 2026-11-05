#pragma once

#include <optional>
#include <string>
#include <vector>

// конфиг очереди из YAML конфига
struct QueueConfig {
    std::string name;
    std::string durability = "memory";  // memory | disk
    std::string order = "fifo";         // unordered | fifo | sorted
    std::string sortKey;                // используется только для order == "sorted"
    std::optional<int> messageTtl;      // секунды

    bool isDisk() const { 
        return durability == "disk"; 
    }
};

// конфиг брокера
struct BrokerConfig {
    std::string bind = "tcp://*:5555";
    std::string dataDir = "data";
    std::vector<QueueConfig> queues;

    static BrokerConfig fromFile(const std::string& path);
};
