#include <utility>

#include <broker/FifoMemoryQueue.hpp>

FifoMemoryQueue::FifoMemoryQueue(std::string name) : name_(std::move(name)) {}

const std::string& FifoMemoryQueue::name() const {
    return name_;
}

void FifoMemoryQueue::push(Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    messages_.push_back(std::move(message));
}

std::optional<Message> FifoMemoryQueue::at(std::size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (index >= messages_.size()) {
        return std::nullopt;
    }

    return messages_[index];
}

std::size_t FifoMemoryQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return messages_.size();
}
