#include <iostream>
#include <thread>

#include <tasks/PickingTask.h>
#include <workers/Loader.h>

PickingTask::PickingTask(const Order& order, int maxLoaders) : order_(order), maxLoaders_(maxLoaders) {}

void PickingTask::execute(Loader& loader) {
    std::cout << "Грузчик #" << loader.id() << " комплектует заказ #" << order_.id << " в город " << order_.city
        << " (до " << maxLoaders_ << " грузчиков)\n";
    std::this_thread::sleep_for(loader.actionTime());
}

std::string PickingTask::name() const {
    return "Комплектация";
}
