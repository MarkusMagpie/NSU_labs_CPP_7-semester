#include <iostream>
#include <thread>

#include <tasks/UnloadTask.h>
#include <workers/Loader.h>

UnloadTask::UnloadTask(const Truck& truck) : truck_(truck) {}

void UnloadTask::execute(Loader& loader) {
    // std::cout << "Разгружаю машину #" << truck_.id << " (" << truck_.pallets.size() << " паллет)\n";
    std::cout << "Грузчик #" << loader.id() << " разгружает машину #" << truck_.id << '\n';
    std::this_thread::sleep_for(loader.actionTime());  // действие "взять паллету"
}

std::string UnloadTask::name() const {
    return "Разгрузка";
}
