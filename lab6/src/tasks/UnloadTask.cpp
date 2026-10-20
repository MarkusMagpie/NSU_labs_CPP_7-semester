#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <tasks/UnloadTask.h>
#include <utils/Logger.h>
#include <workers/Loader.h>

UnloadTask::UnloadTask(const Truck& truck, StorageZone& storage) : truck_(truck), storage_(storage) {}

void UnloadTask::execute(Loader& loader) {
    std::string who = "Грузчик #" + std::to_string(loader.id());
    std::string truckName = "машины #" + std::to_string(truck_.id);
    Logger::log(who + " начал разгрузку " + truckName + " (" + std::to_string(truck_.pallets.size()) + " паллет)");

    for (Pallet& pallet : truck_.pallets) {
        // loader получил флаг о завершении работы потока из Warehouse
        if (loader.stopRequested()) {
            Logger::log(who + " прервал разгрузку " + truckName + ", так как его смена закончилась");
            return;
        }

        // 1 взять товар на паллете из машины
        std::this_thread::sleep_for(loader.actionTime());

        // 2 на терминале ввести вид и количество -> получить адрес(а) хранения
        std::vector<StorageZone::Placement> placements = storage_.requestPlacement(pallet.type, pallet.count);
        std::this_thread::sleep_for(loader.actionTime());

        // 3 переместить товар в зону хранения по адресу
        std::this_thread::sleep_for(loader.actionTime());

        // 4 отметить на терминале зоны хранения что товар размещен
        for (StorageZone::Placement& placement : placements) {
            storage_.confirmPlacement(placement);
        }
        std::this_thread::sleep_for(loader.actionTime());

        // лог: какой грузчик что куда положил
        std::ostringstream message;
        message << who << " разместил паллету из " << truckName << " (" << pallet << "):";
        int placed = 0;
        for (const StorageZone::Placement& placement : placements) {
            message << ' ' << placement.address << " -> " << placement.count;
            placed += placement.count;
        }
        if (placed < pallet.count) {
            message << " | НЕ ХВАТИЛО МЕСТА для " << pallet.count - placed << " шт";
        }
        Logger::log(message.str());
    }

    Logger::log(who + " закончил разгрузку " + truckName);
}

std::string UnloadTask::name() const {
    return "Разгрузка";
}
