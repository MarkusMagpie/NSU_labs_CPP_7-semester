#include <iostream>

#include <Warehouse.h>

Warehouse::Warehouse(const Config& config)
    : config_(config),
      receivingDock_(config.receivingSlots),
      storageZone_(config.storageRacks, config.storageTerminals),
      packingZone_(config.packingTables),
      shippingZone_(config.shippingSlots) {
    std::cout << "Склад создан\n";
}
