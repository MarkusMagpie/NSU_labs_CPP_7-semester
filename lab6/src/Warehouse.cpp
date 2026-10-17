#include <iostream>

#include <Warehouse.h>
#include <utils/RandomGenerator.h>

using namespace limits;

Warehouse::Warehouse(Config& config)
    : config_(config),
      receivingDock_(config.receivingSlots),
      storageZone_(config.storageRacks, config.storageTerminals),
      packingZone_(config.packingTables),
      shippingZone_(config.shippingSlots) {
    auto gen = RandomGenerator::getGenerator();

    // определение скорости действий каждого грузчика
    for (int i = 0; i < config.loaders; ++i) {
        int units = RandomGenerator::getRandom(gen, kMinActionTime, kMaxActionTime);
        loaders_.push_back(Loader(i, config.timeUnit() * units));
    }
    std::cout << "Создано грузчиков: " << loaders_.size()
              << " (например у грузчика #0 действие длится " << loaders_[0].actionTime().count() << " мс)\n";

    // менеджеры не имеют по ТЗ никаких характеристик наподобии скорости действий
    for (int i = 0; i < config.managers; ++i) {
        managers_.push_back(Manager(i));
    }
    std::cout << "Создано менеджеров: " << managers_.size() << '\n';

    std::cout << "Склад/Warehouse создан\n";
}
