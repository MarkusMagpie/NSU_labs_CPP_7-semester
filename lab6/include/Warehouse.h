#pragma once

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
    
    TaskQueue taskQueue_;  // общая очередь задач для всех грузчиков

    std::vector<Loader> loaders_;
    std::vector<Manager> managers_;
public:
    Warehouse(Config& config);
};
