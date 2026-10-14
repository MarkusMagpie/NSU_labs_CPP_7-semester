#pragma once

#include <tasks/TaskQueue.h>
#include <utils/Config.h>
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
public:
    Warehouse(const Config& config);
};
