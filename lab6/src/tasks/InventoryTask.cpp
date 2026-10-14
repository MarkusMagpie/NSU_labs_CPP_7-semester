#include <iostream>

#include <tasks/InventoryTask.h>

InventoryTask::InventoryTask(const std::vector<ProductType>& types) : types_(types) {}

void InventoryTask::execute() {
    std::cout << "Провожу инвентаризацию видов товара:";
    for (ProductType type : types_) {
        std::cout << ' ' << type;
    }
    std::cout << '\n';
}

std::string InventoryTask::name() const {
    return "Инвентаризация";
}
