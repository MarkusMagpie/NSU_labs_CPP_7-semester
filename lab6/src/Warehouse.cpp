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
      shippingZone_(config.shippingSlots),
      tasksLeft_(100) {
    auto gen = RandomGenerator::getGenerator();

    // отдых грузчика по ТЗ равен максимальному времени выполнения одной задачи;
    // пока задача-заглушка = одно действие, а действие длится максимум kMaxActionTime единиц
    auto restTime = config.timeUnit() * kMaxActionTime;

    // определение скорости действий каждого грузчика
    for (int id = 0; id < config.loaders; ++id) {
        int units = RandomGenerator::getRandom(gen, kMinActionTime, kMaxActionTime);
        // emplace_back создает Loader прямо внутри deque из этих аргументов, без копирования
        loaders_.emplace_back(id, config.timeUnit() * units, restTime, taskQueue_);
    }
    std::cout << "Создано грузчиков: " << loaders_.size() << "\n";

    // как часто менеджер ставит задачи, в ТЗ не задано: беру тот же диапазон 1-5 единиц времени
    for (int id = 0; id < config.managers; ++id) {
        int units = RandomGenerator::getRandom(gen, kMinActionTime, kMaxActionTime);
        managers_.emplace_back(id, config.timeUnit() * units, config.cities, taskQueue_, receivingDock_, storageZone_, shippingZone_, tasksLeft_);
    }
    std::cout << "Создано менеджеров: " << managers_.size() << '\n';

    std::cout << "Склад/Warehouse создан\n";
}

void Warehouse::start() {
    Logger::log("=== Склад начинает работу ===");
    for (auto& manager : managers_) {
        manager.start_thread();
    }

    for (auto& loader : loaders_) {
        loader.start_thread();
    }
}

void Warehouse::stop() {
    Logger::log("=== Склад останавливается ===");

    // одновременно прошу всех рабочих остановиться
    for (auto& manager : managers_) {
        manager.request_stop();
    }
    for (auto& loader : loaders_) {
        loader.request_stop();
    }

    // main ждет в порядке массива завершения работы потока
    for (auto& manager : managers_) {
        manager.join();
    }
    for (auto& loader : loaders_) {
        loader.join();
    }

    Logger::log("=== Склад остановлен: все потоки завершены ===");
}

TaskQueue& Warehouse::taskQueue() {
    return taskQueue_;
}
