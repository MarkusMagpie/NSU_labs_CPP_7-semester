#include <sstream>
#include <thread>

#include <tasks/InventoryTask.h>
#include <utils/Logger.h>
#include <workers/Loader.h>

InventoryTask::InventoryTask(const std::vector<ProductType>& types) : types_(types) {}

void InventoryTask::execute(Loader& loader) {
    // собираем всю строку заранее (ostringstream умеет << для ProductType), а выводим одним вызовом
    std::ostringstream message;
    message << "Грузчик #" << loader.id() << " проводит инвентаризацию видов товара:";
    for (ProductType type : types_) {
        message << ' ' << type;
    }
    Logger::log(message.str());

    std::this_thread::sleep_for(loader.actionTime());
}

std::string InventoryTask::name() const {
    return "Инвентаризация";
}
