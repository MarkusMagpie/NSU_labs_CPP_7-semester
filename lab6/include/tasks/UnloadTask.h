#pragma once

#include <mutex>
#include <optional>

#include <model/Truck.h>
#include <tasks/Task.h>
#include <zones/ReceivingDock.h>
#include <zones/StorageZone.h>

// ЗАДАЧА НА РАЗГРУЗКУ: перенести все паллеты из машины в зону хранения.
// Машина - общее состояние для всех грузчиков, которые ее разгружают (в 6.3 их станет несколько)
class UnloadTask : public Task {
private:
    Truck truck_; // машина которую нужно разгрузить
    int slot_; // слот зоны приемки, в котором стоит машина (занял менеджер)
    ReceivingDock& dock_; // зона приемки: освободить слот после разгрузки
    StorageZone& storage_; // куда перенести товар (зона хранения склада)

    std::mutex pallet_mutex_; // защищает счетчики nextPallet_ и placedPallets_
    int nextPallet_ = 0; // индекс первой не взятой паллеты в truck_.pallets
    int placedPallets_ = 0; // сколько паллет уже размещено полностью в зоне хранения

    // действие 1 "взять паллету из машины" отдает следующую невзятую паллету
    // или пустой optional, если невзятых паллет не осталось
    std::optional<Pallet> takePallet();

    // палета полностью размещена?; true если это была последняя паллета машины
    bool markPlaced();
public:
    UnloadTask(const Truck& truck, int slot, ReceivingDock& dock, StorageZone& storage);

    void execute(Loader& loader) override;
    std::string name() const override;
};
