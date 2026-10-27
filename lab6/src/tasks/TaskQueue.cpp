#include <tasks/TaskQueue.h>

void TaskQueue::push(std::shared_ptr<Task> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()
        tasks_.push_back(task);
    } // mutex.unlock()

    // бужу одного спящего в pop() безработного рабочего, 
    // даю ему возможность захватить мьютекс
    taskAdded_.notify_one();
}

std::shared_ptr<Task> TaskQueue::pop(std::stop_token stopToken, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    // есть задачи в очереди? просили остановиться? время ожидания вышло? -> false
    bool hasTask = taskAdded_.wait_for(lock, 
        stopToken, 
        timeout, 
        [this] { return !tasks_.empty(); });

    if (!hasTask) {
        return nullptr; // таймаут или просьба остановиться
    } else {
        std::shared_ptr<Task> task = tasks_.front(); // дождался задачи 
        tasks_.pop_front();

        return task;
    }
}

int TaskQueue::size() {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()
    return static_cast<int>(tasks_.size());
}
