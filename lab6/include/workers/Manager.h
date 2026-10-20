#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <stop_token>
#include <thread>

#include <tasks/TaskQueue.h>
#include <zones/ReceivingDock.h>
#include <zones/StorageZone.h>

// МЕНЕДЖЕР: ставит задачи для работы склада (разгрузка, инвентаризация, комплектация)
// и оповещает о приходе машин для отправки заказов
class Manager {
private:
    int id_;
    std::chrono::milliseconds period_; // как часто менеджер ставит задачи (в ТЗ не задано)
    int cities_; // сколько городов на складе (для заказов)
    TaskQueue& queue_; // очередь задач склада
    ReceivingDock& dock_; // зона приемки: машина должна занять слот до постановки задачи разгрузки
    StorageZone& storage_; // зона хранения склада; нужна задачам разгрузки
    std::jthread thread_;

    // для ВСЕХ менеджеров счетчик номеров машин и заказов; не повторяются
    // atomic - потому что его одновременно увеличивают потоки разных менеджеров
    static std::atomic<int> nextId_;

    // создать случайную задачу; nullptr, если задачу поставить не удалось (нет свободного слота приемки)
    std::shared_ptr<Task> createRandomTask(std::mt19937& gen, std::stop_token stopToken);
public:
    Manager(int id, 
        std::chrono::milliseconds period, 
        int cities,
        TaskQueue& queue, 
        ReceivingDock& dock, 
        StorageZone& storage);

    int id() const;

    // запустить поток менеджера; поток выполняет run()
    void start_thread();
    // попросить поток остановиться
    void request_stop();
    // дождаться завершения потока
    void join();

    void run(std::stop_token stopToken);
};
