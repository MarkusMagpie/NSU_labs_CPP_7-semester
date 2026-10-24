#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <stop_token>
#include <future>
#include <thread>
#include <utility>
#include <vector>

#include <tasks/InventoryTask.h>
#include <tasks/TaskQueue.h>
#include <zones/ReceivingDock.h>
#include <zones/ShippingZone.h>
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
    ShippingZone& shipping_; // зона отправки: сюда менеджер ставит приехавшие машины для отправки
    // общий для всех менеджеров лимит: сколько задач еще можно поставить за смену (принадлежит Warehouse)
    std::atomic<int>& tasksLeft_;
    
    // еще не полученные отчеты инвентаризации
    std::vector<std::pair<int, std::future<InventoryTask::Reports>>> pendingReports_;

    std::jthread thread_; // последним: поток работает с полями выше, они должны жить дольше него

    // для ВСЕХ менеджеров счетчик номеров машин и заказов; не повторяются
    // atomic - потому что его одновременно увеличивают потоки разных менеджеров
    static std::atomic<int> nextId_;

    // создать случайную задачу; nullptr, если задачу поставить не удалось (нет свободного слота приемки)
    std::shared_ptr<Task> createRandomTask(std::mt19937& gen, std::stop_token stopToken);

    // забрать и вывести уже готовые отчеты инвентаризации (не ждет неготовые)
    void collectReadyReports();

    // оповестить о приходе машины для отправки: поставить ее в свободный слот зоны отправки
    void announceShippingTruck(std::mt19937& gen);
public:
    Manager(int id, 
        std::chrono::milliseconds period, 
        int cities,
        TaskQueue& queue, 
        ReceivingDock& dock, 
        StorageZone& storage,
        ShippingZone& shipping,
        std::atomic<int>& tasksLeft);

    int id() const;

    // запустить поток менеджера; поток выполняет run()
    void start_thread();
    // попросить поток остановиться
    void request_stop();
    // дождаться завершения потока
    void join();

    void run(std::stop_token stopToken);
};
