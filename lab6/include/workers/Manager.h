#pragma once

#include <chrono>
#include <stop_token>
#include <thread>

// МЕНЕДЖЕР: ставит задачи для работы склада (разгрузка, инвентаризация, комплектация)
// и оповещает о приходе машин для отправки заказов
class Manager {
private:
    int id_;
    std::chrono::milliseconds period_; // как часто менеджер ставит задачи (в ТЗ не задано)
    std::jthread thread_;
public:
    Manager(int id, std::chrono::milliseconds period);

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    int id() const;

    // запустить поток менеджера; поток выполняет run()
    void start_thread();
    // попросить поток остановиться
    void request_stop();
    // дождаться завершения потока
    void join();

    void run(std::stop_token stopToken);
};
