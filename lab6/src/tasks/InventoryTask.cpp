#include <iostream>
#include <thread>

#include <tasks/InventoryTask.h>
#include <workers/Loader.h>

InventoryTask::InventoryTask(const std::vector<ProductType>& types) : types_(types) {}

void InventoryTask::execute(Loader& loader) {
    std::cout << "Грузчик #" << loader.id() << " проводит инвентаризацию видов товара:";
    for (ProductType type : types_) {
        std::cout << ' ' << type;
    }
    std::cout << '\n';
    std::this_thread::sleep_for(loader.actionTime());
}

std::string InventoryTask::name() const {
    return "Инвентаризация";
}
