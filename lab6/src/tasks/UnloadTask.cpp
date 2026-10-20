#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <tasks/UnloadTask.h>
#include <utils/Logger.h>
#include <workers/Loader.h>

UnloadTask::UnloadTask(const Truck& truck, int slot, ReceivingDock& dock, StorageZone& storage)
    : truck_(truck), slot_(slot), dock_(dock), storage_(storage) {}

std::optional<Pallet> UnloadTask::takePallet() {
    std::lock_guard<std::mutex> lock(pallet_mutex_);

    if (nextPallet_ >= static_cast<int>(truck_.pallets.size())) {
        return std::nullopt; // невзятых паллет не осталось
    }

    return truck_.pallets[nextPallet_++];
}

bool UnloadTask::markPlaced() {
    std::lock_guard<std::mutex> lock(pallet_mutex_);

    ++placedPallets_;
    // машина разгружена, когда все паллеты размещены 
    // (а не когда все взяты - взятые другими грузчиками паллеты могут быть еще в пути)
    return placedPallets_ == static_cast<int>(truck_.pallets.size());
}

void UnloadTask::execute(Loader& loader) {
    std::string who = "Грузчик #" + std::to_string(loader.id());
    std::string truckName = "машины #" + std::to_string(truck_.id);
    Logger::log(who + " начал разгрузку " + truckName + " в слоте " + std::to_string(slot_)
        + " (" + std::to_string(truck_.pallets.size()) + " паллет)");

    while (true) {
        // проверка смены ДО взятия паллеты
        if (loader.stopRequested()) {
            Logger::log(who + " прервал разгрузку " + truckName + ", так как его смена закончилась");
            return;
        }

        // 1 взять товар на паллете из машины
        std::optional<Pallet> taken = takePallet();
        if (!taken) {
            break; // невзятых паллет не осталось
        }
        const Pallet& pallet = *taken;
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

        // размещена последняя паллета машины -> машина разгружена; слот освобождается
        if (markPlaced()) {
            dock_.releaseSlot(slot_);
            Logger::log(who + " завершил разгрузку " + truckName + ", слот " + std::to_string(slot_) + " освобожден");
        }
    }

    Logger::log(who + " закончил работу с " + truckName + ": невзятых паллет не осталось");
}

std::string UnloadTask::name() const {
    return "Разгрузка";
}
