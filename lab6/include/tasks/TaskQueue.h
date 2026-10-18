#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <stop_token>

#include <tasks/Task.h>

// для рассылки задач: менеджеры кладут задачи, грузчики забирают
class TaskQueue {
private:
    std::deque<std::shared_ptr<Task>> tasks_;
    // у каждой очереди свой мьютекс: он защищает именно ее tasks_
    std::mutex mutex_;
    // на ней засыпают грузчики, пока задач нет; 
    // push() будит одного из них.
    // _any - потому что только эта версия умеет просыпаться по stop_token
    std::condition_variable_any taskAdded_;
public:
    // положить задачу в конец очереди и разбудить одного ждущего грузчика
    void push(std::shared_ptr<Task> task);

    /**
     * @brief Ждет задачу и забирает ее из начала очереди
     *
     * Поток рабочего спит, пока не случится одно из трех:
     *  - в очереди появилась задача
     *  - прошло timeout
     *  - у stopToken запросили остановку
     *
     * @param stopToken токен остановки потока
     * @param timeout сколько ждать не больше
     * @return задача или nullptr
     */
    std::shared_ptr<Task> pop(std::stop_token stopToken, std::chrono::milliseconds timeout);

    int size();
};
