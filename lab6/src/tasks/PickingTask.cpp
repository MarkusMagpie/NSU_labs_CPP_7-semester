#include <string>
#include <thread>

#include <tasks/PickingTask.h>
#include <utils/Logger.h>
#include <workers/Loader.h>

PickingTask::PickingTask(const Order& order, int maxLoaders) : order_(order), maxLoaders_(maxLoaders) {}

void PickingTask::execute(Loader& loader) {
    Logger::log("Грузчик #" + std::to_string(loader.id()) + " комплектует заказ #" + std::to_string(order_.id)
                + " в город " + std::to_string(order_.city) + " (до " + std::to_string(maxLoaders_) + " грузчиков)");
    std::this_thread::sleep_for(loader.actionTime());  // заглушка: TODO шаг 9, действия комплектации по ТЗ
}

std::string PickingTask::name() const {
    return "Комплектация";
}
