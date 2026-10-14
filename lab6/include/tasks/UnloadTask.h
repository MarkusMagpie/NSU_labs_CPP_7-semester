#pragma once

#include <model/Truck.h>
#include <tasks/Task.h>

// ЗАДАЧА НА РАЗГРУЗКУ: перенести все паллеты из машины в зону хранения
class UnloadTask : public Task {
private:
    Truck truck_; // машина которую нужно разгрузить
public:
    explicit UnloadTask(const Truck& truck);

    void execute() override;
    std::string name() const override;
};
