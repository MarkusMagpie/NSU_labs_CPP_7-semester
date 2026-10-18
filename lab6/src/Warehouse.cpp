#include <iostream>

#include <Warehouse.h>
#include <utils/Logger.h>
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
        loaders_.push_back(std::make_unique<Loader>(i, config.timeUnit() * units));
    }
    std::cout << "Создано грузчиков: " << loaders_.size()
              << " (например у грузчика #0 действие длится " << loaders_[0]->actionTime().count() << " мс)\n";

    // как часто менеджер ставит задачи, в ТЗ не задано: беру тот же диапазон 1-5 единиц времени
    for (int i = 0; i < config.managers; ++i) {
        int units = RandomGenerator::getRandom(gen, kMinActionTime, kMaxActionTime);
        managers_.push_back(std::make_unique<Manager>(i, config.timeUnit() * units));
    }
    std::cout << "Создано менеджеров: " << managers_.size() << '\n';

    std::cout << "Склад/Warehouse создан\n";
}

void Warehouse::start() {
    Logger::log("=== Склад начинает работу ===");
    for (auto& manager : managers_) {
        manager->start_thread();
    }

    for (auto& loader : loaders_) {
        loader->start_thread();
    }
}

void Warehouse::stop() {
    Logger::log("=== Склад останавливается ===");

    // одновременно прошу всех рабочих остановиться
    for (auto& manager : managers_) {
        manager->request_stop();
    }
    for (auto& loader : loaders_) {
        loader->request_stop();
    }

    // main ждет в порядке массива завершения работы потока
    for (auto& manager : managers_) {
        manager->join();
    }
    for (auto& loader : loaders_) {
        loader->join();
    }

    Logger::log("=== Склад остановлен: все потоки завершены ===");
}
