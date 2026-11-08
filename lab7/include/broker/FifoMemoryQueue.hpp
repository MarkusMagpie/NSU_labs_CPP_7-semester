#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <string>

#include <broker/Message.hpp>

class FifoMemoryQueue {
private:
    const std::string name_;
    std::deque<Message> messages_;

    mutable std::mutex mutex_; // messages_ def
public:
    explicit FifoMemoryQueue(std::string name);

    const std::string& name() const;

    void push(Message message);

    // вернуть копию сообщения по позиции или nullopt если указываемой позиции нет
    std::optional<Message> at(std::size_t index) const;

    std::size_t size() const;
};
