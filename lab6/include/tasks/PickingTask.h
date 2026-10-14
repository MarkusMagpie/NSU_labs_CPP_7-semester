#pragma once

#include <model/Order.h>
#include <tasks/Task.h>

// ЗАДАЧА НА КОМПЛЕКТАЦИЮ: собрать заказ на столе комплектации (в зоне упаковки) и отправить его
class PickingTask : public Task {
private:
    Order order_; // какой заказ собирать
    int maxLoaders_; // сколько грузчиков максимум могут одновременно работать над задачей
public:
    PickingTask(const Order& order, int maxLoaders);

    void execute() override;
    std::string name() const override;
};
