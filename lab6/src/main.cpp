#include <iostream>
#include <string_view>

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

// проверка из ТЗ: от 10 до 100 паллет, на каждой от 1 до palletCapacity единиц
bool isTruckValid(const Truck& truck) {
    int palletCount = static_cast<int>(truck.pallets.size());
    if (palletCount < limits::kMinTruckPallets || palletCount > limits::kMaxTruckPallets) {
        return false;
    }
    
    for (const Pallet& pallet : truck.pallets) {
        if (pallet.count < 1 || pallet.count > palletCapacity(pallet.type)) {
            return false;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    bool fast_mod = argc > 1 && std::string_view(argv[1]) == "--fast";

    Config config = Config::generate(fast_mod);
    std::cout << config << '\n';

    auto gen = RandomGenerator::getGenerator();

    Truck truck = Truck::generate(1, gen);
    std::cout << truck;
    std::cout << "Машина укомплектована корректно: " << (isTruckValid(truck) ? "да" : "НЕТ") << "\n\n";

    Order order = Order::generate(1, config.cities, gen);
    std::cout << order;

    StorageAddress address{3, 2, 7};
    std::cout << "Пример адреса палетты: " << address << "\n\n";

    Warehouse warehouse(config);
    std::cout << '\n';

    TaskQueue queue;
    queue.push(std::make_shared<UnloadTask>(truck));
    queue.push(std::make_shared<InventoryTask>(std::vector<ProductType>{ProductType::Light, ProductType::Heavy}));
    queue.push(std::make_shared<PickingTask>(order, 2));
    std::cout << "задач в очереди: " << queue.size() << '\n';
    // тестовый грузчик: одно действие длится 1 единицу времени
    Loader loader(0, config.timeUnit());
    while (std::shared_ptr<Task> task = queue.tryPop()) {
        std::cout << "[" << task->name() << "] ";
        task->execute(loader);
    }
    std::cout << "задач в очереди: " << queue.size() << "\n\n";

    warehouse.start();
    std::this_thread::sleep_for(config.timeUnit() * 10);
    warehouse.stop();

    return 0;
}
