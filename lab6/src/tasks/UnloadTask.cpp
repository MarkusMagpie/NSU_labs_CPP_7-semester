#include <string>
#include <thread>

#include <tasks/UnloadTask.h>
#include <utils/Logger.h>
#include <workers/Loader.h>

UnloadTask::UnloadTask(const Truck& truck) : truck_(truck) {}

void UnloadTask::execute(Loader& loader) {
    Logger::log("Грузчик #" + std::to_string(loader.id()) + " разгружает машину #" + std::to_string(truck_.id));
    std::this_thread::sleep_for(loader.actionTime());  // заглушка: TODO шаг 6, 4 действия на каждую паллету
}

std::string UnloadTask::name() const {
    return "Разгрузка";
}
