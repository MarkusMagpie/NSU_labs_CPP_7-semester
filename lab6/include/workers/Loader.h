#pragma once

#include <chrono>
#include <stop_token>
#include <thread>

#include <tasks/TaskQueue.h>

// ГРУЗЧИК: берет задачи из общей очереди и выполняет их
class Loader {
private:
    int id_; // номер грузчика
    std::chrono::milliseconds actionTime_; // сколько длится одно действие (1-5 единиц времени)
    std::chrono::milliseconds restTime_; // сколько грузчик ждет задачу
    TaskQueue& queue_; // очередь задач склада (от Warehouse)
    // копия stop_token, полученного run(); пишет и читает ее только поток самого грузчика
    std::stop_token stopToken_;
    std::jthread thread_;
public:
    Loader(int id, 
        std::chrono::milliseconds actionTime, 
        std::chrono::milliseconds restTime, 
        TaskQueue& queue);

    int id() const;
    std::chrono::milliseconds actionTime() const;

    // запустить поток грузчика; поток выполняет run()
    void start_thread();
    // попросить поток остановиться (он заметит это на следующей проверке stop_requested)
    void request_stop();
    // дождаться завершения потока
    void join();
    // попросили ли грузчика остановиться
    bool stopRequested() const;
    // токен остановки для ожиданий внутри задач (bookTable, ship)
    std::stop_token stopToken() const;

    // рабочий цикл грузчика; выполняется в его собственном потоке
    void run(std::stop_token stopToken);
};
