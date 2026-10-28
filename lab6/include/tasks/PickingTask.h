#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <vector>

#include <model/Order.h>
#include <model/Pallet.h>
#include <tasks/Task.h>
#include <zones/PackingZone.h>
#include <zones/ShippingZone.h>
#include <zones/StorageZone.h>

// ЗАДАЧА НА КОМПЛЕКТАЦИЮ: собрать заказ на столе комплектации (в зоне упаковки) и отправить его
class PickingTask : public Task {
private:
    Order order_; // какой заказ собирать: город и сколько единиц каждого вида
    int maxLoaders_; // сколько грузчиков максимум могут одновременно работать над задачей

    // зоны склада с которыми работает комплектация
    StorageZone& storage_; // ЗХ - брать и возвращать паллеты хранения
    PackingZone& packing_; // ЗУ - бронь стола комплектации
    ShippingZone& shipping_; // ЗО - отправка готовых паллет (ship)
    
    std::chrono::milliseconds timeUnit_; // ожидание машины в ship() - 1-5 единиц

    // СОСТОЯНИЕ ЗАКАЗА
    struct OrderProgress {
        // сколько единиц товара каждого вида еще НЕ переложено на паллеты отправки
        std::map<ProductType, int> remaining;
        // сколько единиц каждого вида уже погружено в машины отправки
        std::map<ProductType, int> shipped;
    };

    // СОСТОЯНИЕ СТОЛА КОМПЛЕКТАЦИИ В ЗУ
    struct TableState {
        // забронированный стол
        int table = -1;
        // паллеты хранения (в слоте разбора)
        std::vector<StorageZone::CheckedOutPallet> sortPallets;
        // паллеты отправки (в слоте упаковки)
        std::vector<Pallet> packPallets;
    };

    // один мьютекс на состояния заказа и стола т.к. меняются вместе
    std::mutex state_mutex_;
    
    OrderProgress progress_;
    TableState tableState_;
public:
    PickingTask(const Order& order, 
        int maxLoaders,
        StorageZone& storage, 
        PackingZone& packing, 
        ShippingZone& shipping,
        std::chrono::milliseconds timeUnit);

    void execute(Loader& loader) override;
    std::string name() const override;

    const Order& order() const;
};
