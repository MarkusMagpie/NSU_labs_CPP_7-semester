#pragma once

#include <model/Truck.h>
#include <tasks/Task.h>
#include <zones/StorageZone.h>

// ЗАДАЧА НА РАЗГРУЗКУ: перенести все паллеты из машины в зону хранения
class UnloadTask : public Task {
private:
    Truck truck_; // машина которую нужно разгрузить
    StorageZone& storage_; // куда перенести товар (зона хранения склада)
public:
    UnloadTask(const Truck& truck, StorageZone& storage);

    void execute(Loader& loader) override;
    std::string name() const override;
};
