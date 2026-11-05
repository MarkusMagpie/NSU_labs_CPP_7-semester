#include <stdexcept>

#include <common/YamlConfig.hpp>
#include <yaml-cpp/yaml.h>

namespace {
std::string requireString(const YAML::Node& node, const char* key) {
    if (!node[key]) {
        throw std::runtime_error(std::string("config: отсутствует обязательное поле '") + key + "'");
    }

    return node[key].as<std::string>();
}
} // namespace

BrokerConfig BrokerConfig::fromFile(const std::string& path) {
    // через цель yaml-cpp читает YAML::LoadFile(path)
    YAML::Node root = YAML::LoadFile(path);

    BrokerConfig config;

    if (const YAML::Node broker = root["broker"]) {
        if (broker["bind"]) {
            config.bind = broker["bind"].as<std::string>();
        }
        if (broker["data_dir"]) {
            config.dataDir = broker["data_dir"].as<std::string>();
        }
    }

    const YAML::Node queues = root["queues"];
    if (!queues || !queues.IsSequence()) {
        throw std::runtime_error("config: 'queues' должен быть последовательностью");
    }

    for (const YAML::Node& queue : queues) {
        QueueConfig queueConfig;
        queueConfig.name = requireString(queue, "name");

        if (queue["durability"]) {
            queueConfig.durability = queue["durability"].as<std::string>();
        }
        if (queue["order"]) {
            queueConfig.order = queue["order"].as<std::string>();
        }
        if (queue["sort_key"]) {
            queueConfig.sortKey = queue["sort_key"].as<std::string>();
        }
        if (queue["message_ttl"]) {
            queueConfig.messageTtl = queue["message_ttl"].as<int>();
        }

        config.queues.push_back(std::move(queueConfig));
    }

    return config;
}
