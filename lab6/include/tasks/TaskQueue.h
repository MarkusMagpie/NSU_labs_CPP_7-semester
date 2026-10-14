#pragma once

#include <deque>
#include <memory>

#include <tasks/Task.h>

// для рассылки задач: менеджеры кладут задачи, грузчики забирают.
class TaskQueue {
private:
    std::deque<std::shared_ptr<Task>> tasks_;
public:
    // положить задачу в конец очереди
    void push(std::shared_ptr<Task> task);

    // забрать задачу из начала очереди; если очередь пуста, вернуть nullptr
    std::shared_ptr<Task> tryPop();

    int size() const;
};
