#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <string>
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
    Order order_; // какой заказ собирать
    int maxLoaders_; // максимум могут одновременно работать над задачей

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

    // ШАГИ АЛГОРИТМА КОМПЛЕКТАЦИИ по ТЗ зоны упаковки -------------------

    // "Грузчик #А [заказ #Б]"
    std::string who(const Loader& loader) const;

    // 2 - забронировать стол комплектации
    bool bookTable(Loader& loader);

    // какой вид товара сейчас собирать
    std::optional<ProductType> currentType() const;

    // 3 - забрать паллеты хранения вида type на стол комплектации (ЗХ -> ЗУ)
    void fetchPallets(Loader& loader, ProductType type);

    // 4 - перекладывать по единице с паллет хранения на паллету отправки
    bool moveUnits(Loader& loader, ProductType type);

    // паллета отправки типа type на которую еще можно класть или новая в свободном слоте упаковки
    int packPalletFor(ProductType type);

    // паллета отправки готова: заполнена или этого вида заказу больше не нужно
    bool isReady(const Pallet& pallet) const;

    // 5 - отметить готовность паллеты отправки index и отвезти в ЗО (ship)
    bool shipPallet(Loader& loader, int index);

    // отправить все готовые паллеты отправки (например, оставшиеся на столе после неудачной попытки)
    bool shipReadyPallets(Loader& loader);

    // 6-7 - вернуть паллеты хранения опустевшие или больше не нужные заказу; all - вернуть все
    void returnPallets(Loader& loader, bool all);

    // смена закончилась посреди заказа: убрать за собой:
    // паллеты хранения на полки, бронь стола,
    // резерв еще не взятого товара); 
    // собранные паллеты отправки остаются на столе
    TaskResult stopPicking(Loader& loader);
public:
    PickingTask(const Order& order, 
        int maxLoaders,
        StorageZone& storage, 
        PackingZone& packing, 
        ShippingZone& shipping,
        std::chrono::milliseconds timeUnit);

    TaskResult execute(Loader& loader) override;
    std::string name() const override;

    const Order& order() const;
};
