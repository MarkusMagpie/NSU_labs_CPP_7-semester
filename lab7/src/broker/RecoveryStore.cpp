#include <stdexcept>

#include <broker/DiskStorage.hpp>
#include <broker/RecoveryStore.hpp>

namespace {
std::string encodeId(const std::string& id) {
    const char* digits = "0123456789abcdef";
    std::string result;
    for (unsigned char c : id) {
        result += digits[c >> 4]; // старшие 4 бита
        result += digits[c & 15]; // младшие
    }

    return result;
}

// задать C++ позицию в json
nlohmann::json positionToJson(const RecoveryPosition& position) {
    nlohmann::json result = {
        {"sequence", position.sequence}, 
        {"sort_value", nullptr}
    };

    if (position.sorted) {
        result["sort_value"] = position.sorted->sortValue;
    }

    return result;
}

// считать json позицию в C++
RecoveryPosition readPosition(const nlohmann::json& value, QueueOrder order) {
    if (!value.is_object() 
        || !value.contains("sequence") 
        || !value["sequence"].is_number_unsigned()
        || !value.contains("sort_value")) {
        throw std::runtime_error("Некорректная позиция recovery");
    }

    RecoveryPosition position;
    position.sequence = value.at("sequence").get<std::uint64_t>();
    const auto& key = value.at("sort_value");

    if (order == QueueOrder::Sorted && position.sequence != 0) {
        if (!key.is_number_integer()) {
            throw std::runtime_error("Некорректный ключ позиции recovery");
        }

        position.sorted = SortedReadPosition{key, position.sequence};
    } else if (!key.is_null()) {
        throw std::runtime_error("Неожиданный ключ позиции recovery");
    }

    return position;
}

// путь к файлу состояния: каталог очереди / recovery / закодированный_client_id.json
std::filesystem::path statePath(const Queue& queue, const std::string& consumerId) {
    return *queue.diskDirectory() / "recovery" / (encodeId(consumerId) + ".json");
}
} // namespace

// возвращает состояние конкретного потребителя в конкретной очереди
RecoveryState RecoveryStore::load(const Queue& queue, const std::string& consumerId) {
    const auto key = std::make_pair(queue.name(), consumerId); // например {"Orders", "reader-1"}
    const auto existing = states_.find(key);

    // если RecoveryState уже загружено в память брокера
    if (existing != states_.end()) {
        return existing->second;
    }

    RecoveryState state;
    // если очередь durability: disk и состояние еще не загружено -> прочитать файл
    if (queue.diskDirectory()) {
        const auto path = statePath(queue, consumerId);
        if (std::filesystem::exists(path)) {
            const auto value = DiskStorage::readJson(path);
            if (!value.is_object() 
                || value.value("consumer_id", std::string{}) != consumerId
                || value.value("queue", std::string{}) != queue.name()
                || value.value("order", std::string{}) != (queue.order() == QueueOrder::Sorted ? "sorted" : "sequence")) {
                throw std::runtime_error("Некорректный файл recovery: " + path.string());
            }

            state.confirmed = readPosition(value.at("confirmed"), queue.order());
            /*
            "pending": {
                "msg_id": "12",
                "position": {
                    "sequence": 5,
                    "sort_value": null
                }
            }
            */
            if (!value.at("pending").is_null()) {
                // извлекаю поля pending, msg_id, position
                const auto& pending = value.at("pending");
                const auto messageId = pending.at("msg_id").get<std::string>();
                auto position = readPosition(pending.at("position"), queue.order());
                
                if (messageId.empty() || position.sequence == 0) {
                    throw std::runtime_error("Некорректная неподтвержденная выдача: " + path.string());
                }

                /*
                state.pending.messageId = "12"
                state.pending.position.sequence = 5
                */
                state.pending = RecoveryPending{messageId, std::move(position)};
            }
        }
    }
    states_.emplace(key, state);
    
    return state;
}

void RecoveryStore::save(const Queue& queue, const std::string& consumerId, const RecoveryState& state) {
    // для очереди с durability: disk
    if (queue.diskDirectory()) {
        // в data/Orders/recovery/ появится файл типа 7265616465722d31.json
        const auto path = statePath(queue, consumerId);
        std::filesystem::create_directories(path.parent_path()); // data/Orders/recovery
        
        nlohmann::json pending = nullptr;
        if (state.pending) {
            pending = {
                {"msg_id", state.pending->messageId}, 
                {"position", positionToJson(state.pending->position)}
            };
        }
        /*
        "pending": {
            "msg_id": "12",
            "position": {
                "sequence": 5,
                "sort_value": null
            }
        }
        */
        
        DiskStorage::writeJson(path, 
            {{"queue", queue.name()}, 
                {"consumer_id", consumerId},
                {"order", queue.order() == QueueOrder::Sorted ? "sorted" : "sequence"},
                {"confirmed", positionToJson(state.confirmed)}, 
                {"pending", pending}
            });
    }

    // для очереди с durability: memory - обновление состояния в памяти
    states_.insert_or_assign(std::make_pair(queue.name(), consumerId), state);
}
