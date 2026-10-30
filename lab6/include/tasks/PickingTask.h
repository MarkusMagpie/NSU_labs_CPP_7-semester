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

    // паллета отправки на столе; id чтобы найти ее после ship(): пока ее везли, другие грузчики
    // могли удалить из вектора другие паллеты и индекс сдвинулся
    struct PackPallet {
        int id = 0;
        Pallet pallet;
        bool shipping = false; // ее сейчас везет в ЗО какой-то грузчик (слот упаковки она все еще занимает)
    };

    // СОСТОЯНИЕ СТОЛА КОМПЛЕКТАЦИИ В ЗУ
    struct TableState {
        // забронированный стол
        int table = -1;
        // какой-то грузчик сейчас бронирует стол -> остальные ждут
        bool booking = false;
        // паллеты хранения (в слоте разбора)
        std::vector<StorageZone::CheckedOutPallet> sortPallets;
        // паллеты хранения, которые сейчас несут на стол: слот разбора уже за ними, товар уже "наш"
        int incomingPallets = 0;
        std::map<ProductType, int> incomingUnits;
        // паллеты отправки (в слоте упаковки)
        std::vector<PackPallet> packPallets;
        int nextPackId = 0;
    };

    // защищает поля ниже
    std::mutex state_mutex_;

    OrderProgress progress_;
    TableState tableState_;
    int activeLoaders_ = 0; // сколько грузчиков сейчас выполняют задачу
    bool finished_ = false; // заказ собран или прерван остановкой: присоединяться больше нельзя

    // почему грузчик уходит с задачи
    enum class LeaveReason {
        NoWork, // собирать больше нечего (последний -> заказ готов)
        Failed, // не дали стол / не отправил паллету за 5 попыток (последний -> задачу в очередь)
        Stop // смена закончилась (последний -> убрать за собой)
    };

    // чем закончилась попытка переложить единицу
    enum class MoveResult {
        Moved, // переложил
        NoUnits, // на столе нет паллет хранения этого вида с товаром
        NoPackSlot // оба слота упаковки заняты (паллеты везут в ЗО)
    };

    // ШАГИ АЛГОРИТМА КОМПЛЕКТАЦИИ по ТЗ зоны упаковки -------------------
    // !!! ...Locked - вызывать только под захваченным state_mutex_

    // "Грузчик #А [заказ #Б]"
    std::string who(const Loader& loader) const;

    // 1 - встать на задачу если на ней меньше maxLoaders_ грузчиков и заказ не закончен
    bool tryJoin(Loader& loader);

    // уйти с задачи; последний ушедший решает судьбу задачи (см. LeaveReason)
    TaskResult leave(Loader& loader, LeaveReason reason);

    // 2 - забронировать стол комплектации
    bool bookTable(Loader& loader);

    // какой вид товара сейчас собирать
    std::optional<ProductType> currentTypeLocked() const;

    // 3 - начать нести паллету хранения вида type (занять слот разбора); nullopt + причина - нести нечего
    std::optional<StorageZone::CheckedOutPallet> startFetchLocked(ProductType type, std::string& waitReason);
    // 3 - донести паллету и поставить на стол (3 действия)
    void finishFetch(Loader& loader, const StorageZone::CheckedOutPallet& pallet);

    // 4 - переложить одну единицу с паллеты хранения на паллету отправки (изменение состояния)
    MoveResult moveUnitLocked(ProductType type);

    // паллета отправки типа type на которую еще можно класть или новая в свободном слоте упаковки
    int packPalletForLocked(ProductType type);

    // паллета отправки готова: заполнена или этого вида заказу больше не нужно
    bool isReadyLocked(const Pallet& pallet) const;

    // 5 - готовая паллета отправки которую еще никто не везет -> "везу я"
    std::optional<PackPallet> claimReadyPackPalletLocked();
    // 5 - отметить готовность и отвезти в ЗО (ship); false - не отправлена, паллета снова на столе
    bool shipPallet(Loader& loader, const PackPallet& packPallet);

    // 6-7 - паллета хранения опустела или заказу этот вид больше не нужен -> снять ее со стола комп
    std::optional<StorageZone::CheckedOutPallet> takeUselessSortPalletLocked();
    // 6-7 - вернуть паллету хранения на полку
    void returnPallet(Loader& loader, const StorageZone::CheckedOutPallet& pallet);
public:
    PickingTask(const Order& order, 
        int maxLoaders,
        StorageZone& storage, 
        PackingZone& packing, 
        ShippingZone& shipping,
        std::chrono::milliseconds timeUnit);

    TaskResult execute(Loader& loader) override;
    std::string name() const override;

    // есть ли место еще для одного грузчика? (кроме того кто сейчас вынул задачу из очереди)
    bool needsMoreLoaders() override;

    const Order& order() const;
};
