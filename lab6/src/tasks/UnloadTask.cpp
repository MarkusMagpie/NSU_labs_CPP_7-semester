#include <iostream>

#include <tasks/UnloadTask.h>

UnloadTask::UnloadTask(const Truck& truck) : truck_(truck) {}

void UnloadTask::execute() {
    std::cout << "Разгружаю машину #" << truck_.id << " (" << truck_.pallets.size() << " паллет)\n";
}

std::string UnloadTask::name() const {
    return "Разгрузка";
}
