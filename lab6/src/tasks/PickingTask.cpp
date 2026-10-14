#include <iostream>

#include <tasks/PickingTask.h>

PickingTask::PickingTask(const Order& order, int maxLoaders) : order_(order), maxLoaders_(maxLoaders) {}

void PickingTask::execute() {
    std::cout << "Комплектую заказ #" << order_.id << " в город " << order_.city 
        << " (до " << maxLoaders_ << " грузчиков)\n";
}

std::string PickingTask::name() const {
    return "Комплектация";
}
