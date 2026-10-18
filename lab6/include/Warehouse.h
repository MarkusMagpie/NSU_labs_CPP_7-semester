#pragma once

#include <memory>
#include <vector>

#include <tasks/TaskQueue.h>
#include <utils/Config.h>
#include <workers/Loader.h>
#include <workers/Manager.h>
#include <zones/PackingZone.h>
#include <zones/ReceivingDock.h>
#include <zones/ShippingZone.h>
#include <zones/StorageZone.h>

// склад: владеет всеми четырьмя зонами, очередью задач, грузчиками и менеджерами
class Warehouse {
private:
    Config config_;

    ReceivingDock receivingDock_;
    StorageZone storageZone_;
    PackingZone packingZone_;
    ShippingZone shippingZone_;
    
    TaskQueue taskQueue_; // общая очередь задач для всех грузчиков

    // при росте вектора будут переезжать указатели,
    // а объекты Loader останутся на прежних местах в куче
    std::vector<std::unique_ptr<Loader>> loaders_;
    std::vector<std::unique_ptr<Manager>> managers_;
public:
    Warehouse(Config& config);

    // запустить потоки рабочих
    void start();
    // попросить рабочих остановиться и дождаться завершения всех потоков
    void stop();
};
