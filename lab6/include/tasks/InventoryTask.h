#pragma once

#include <vector>

#include <model/ProductType.h>
#include <tasks/Task.h>

// ЗАДАЧА НА ИНВЕНТАРИЗАЦИЮ: получить отчеты по указанным видам товара из зоны хранения
class InventoryTask : public Task {
private:
    std::vector<ProductType> types_; // по каким видам товара нужен отчет
public:
    explicit InventoryTask(const std::vector<ProductType>& types);

    void execute() override;
    std::string name() const override;
};
