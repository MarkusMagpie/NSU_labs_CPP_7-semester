#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <string>

#include <broker/Message.hpp>

enum class QueueOrder {
    Fifo,
    Unordered
};

class MemoryQueue {
private:
    const std::string name_;
    const QueueOrder order_;
    std::deque<Message> messages_;

    mutable std::mutex mutex_; // messages_ def
public:
    MemoryQueue(std::string name, QueueOrder order);

    const std::string& name() const;
    QueueOrder order() const;

    void push(Message message);

    std::optional<Message> at(std::size_t index) const;

    std::size_t size() const;
};
