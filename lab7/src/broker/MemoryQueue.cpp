#include <utility>

#include <broker/MemoryQueue.hpp>

MemoryQueue::MemoryQueue(std::string name, QueueOrder order)
    : name_(std::move(name)), order_(order) {}

const std::string& MemoryQueue::name() const {
    return name_;
}

QueueOrder MemoryQueue::order() const {
    return order_;
}

void MemoryQueue::push(Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    messages_.push_back(std::move(message));
}

std::optional<Message> MemoryQueue::at(std::size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (index >= messages_.size()) {
        return std::nullopt;
    }

    return messages_[index];
}

std::size_t MemoryQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return messages_.size();
}
