#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>

#include <broker/Queue.hpp>

// позиции чтения
struct RecoveryPosition {
    // fifo + unordered
    std::uint64_t sequence = 0;
    // sorted
    std::optional<SortedReadPosition> sorted; // последнее письмо полученное потребителем в очереди с sorted order (key, sequence)
};

struct RecoveryPending {
    std::string messageId;
    RecoveryPosition position;
};

// состояние для пары client_id потребителя + очередь
struct RecoveryState {
    RecoveryPosition confirmed; // last acke-ed position
    std::optional<RecoveryPending> pending; // выданное брокером но еще не ack-ed position
};

/*
position = RecoveryState {
    confirmed = {
        sequence = 4
    },
    pending = {
        messageId = "12",
        position = {
            sequence = 5
        }
    }
}
*/



class RecoveryStore {
public:
    RecoveryState load(const Queue& queue, const std::string& consumerId);
    void save(const Queue& queue, const std::string& consumerId, const RecoveryState& state);
private:
    /*
    {"Orders", "reader-1"} -> RecoveryState
    {"Orders", "reader-2"} -> другое RecoveryState
    {"Logs", "reader-1"} -> еще RecoveryState
    */
    std::map<
        std::pair<std::string, std::string>, // {имя очереди, client_id}
        RecoveryState
    > states_;
};
