#include <tasks/TaskQueue.h>

void TaskQueue::push(std::shared_ptr<Task> task) {
    tasks_.push_back(task);
}

std::shared_ptr<Task> TaskQueue::tryPop() {
    if (tasks_.empty()) {
        return nullptr;
    }
    
    std::shared_ptr<Task> task = tasks_.front();
    tasks_.pop_front();
    return task;
}

int TaskQueue::size() const {
    return static_cast<int>(tasks_.size());
}
