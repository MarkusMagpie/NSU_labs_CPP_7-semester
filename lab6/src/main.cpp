#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

#include <Warehouse.h>
#include <model/Order.h>
#include <model/StorageAddress.h>
#include <model/Truck.h>
#include <tasks/InventoryTask.h>
#include <tasks/PickingTask.h>
#include <tasks/TaskQueue.h>
#include <tasks/UnloadTask.h>
#include <utils/Config.h>
#include <utils/Logger.h>
#include <utils/RandomGenerator.h>
#include <workers/Loader.h>

int main(int argc, char* argv[]) {
    bool fast_mod = argc > 1 && std::string_view(argv[1]) == "--fast";

    Config config = Config::generate(fast_mod);

    Warehouse warehouse(config);

    warehouse.start();
    std::this_thread::sleep_for(config.timeUnit() * 120);
    warehouse.stop();

    Logger::log("в очереди склада осталось задач: " + std::to_string(warehouse.taskQueue().size()));

    return 0;
}
