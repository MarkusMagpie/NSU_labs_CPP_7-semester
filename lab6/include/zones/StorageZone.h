#pragma once

#include <array>
#include <vector>
#include <map>
#include <mutex>
#include <optional>

#include <model/ProductType.h>
#include <model/StorageAddress.h>
#include <utils/Config.h>

/*
ЗОНА ХРАНЕНИЯ: от 10 до 500 стеллажей, на стеллаже от 1 до 6 полок, 
на полке 10 мест под паллеты;
на паллете только один вид товара не больше его лимита (100 / 30 / 4);
от 1 до 10 терминалов системы учета;
*/
class StorageZone {
public:
    StorageZone(int rackCount, int terminalCount);

    int rackCount() const;
    int terminalCount() const;
    int totalPlaces() const;

    // ответ терминала: по какому адресу положить сколько единиц товара
    struct Placement {
        StorageAddress address; // куда положить
        int count = 0; // сколько единиц
    };

    // для задачи на инвентаризацию
    struct ProductStats {
        int receivedInReceivingDock = 0;    // принято в зоне приемки (только растет)
        int inStorageZone = 0;              // лежит в зоне хранения
        int inPackingZone = 0;              // в процессе упаковки
        int shipped = 0;                    // отправлено (только растет)
    };
    ProductStats stats(ProductType type) const;

    // действие 2 приемки: "ввести вид и количество -> получить адрес"
    // На терминале системы учета ввести количество и вид товара на паллете и получить адрес (или адреса) хранения товара в зоне хранения
    std::vector<Placement> requestPlacement(ProductType type, int count);

    // действие 2 приемки: товар отсканирован на терминале слота приемки -> он принят
    void registerReceived(ProductType type, int count);

    // действие 4: "отметить что товар размещен"
    // Отметить в терминале системы учета в зоне хранения, что товар был размещен по заданному адресу
    void confirmPlacement(Placement& placement);

    // ИНВЕНТОРИЗАЦИЯ -------------------------------------------------------------------
    // одна паллета в отчете: где стоит и сколько на ней товара
    struct PalletInfo {
        StorageAddress address;
        int count = 0;
    };

    // отчет по одному виду товара
    struct ProductReport {
        ProductType type;
        ProductStats stats; 
        std::vector<PalletInfo> pallets; // все паллеты с этим товаром (их адреса и к-во)
    };

    ProductReport report(ProductType type) const;

    // КОМПЛЕКТАЦИЯ ---------------------------------------------------------------------
    // Путь товара заказа: 
    // полка -> (checkoutPallet) стол комплектации -> 
    // (takeUnits) паллета отправки
    // -> машина отправки -> (registerShipped) отправлено. 
    // Оставшееся на паллете хранения -> (returnPallet) на полку

    /**
     * @brief резерв товара под заказ (вызывает менеджер ДО постановки задачи на комплектацию)
     *
     * @param items сколько единиц каждого вида нужно заказу
     * @return true - зарезервировано, заказ можно ставить; false - товара не хватает
     */
    bool reserveForOrder(const std::map<ProductType, int>& items);

    // снять резерв если заказ не будет собран
    void releaseReservation(const std::map<ProductType, int>& items);

    // паллета хранения выданная на стол комплектации в зоне упаковки
    struct CheckedOutPallet {
        StorageAddress address; // откуда взята
        ProductType type = ProductType::Light;
        int count = 0; // сколько единиц товара было на паллете в момент выдачи
    };

    /**
     * @brief шаг 3 комплектации: выдать паллету хранения с полки на стол 
     * (зона хранения -> зона упаковки)
     */
    std::optional<CheckedOutPallet> checkoutPallet(ProductType type);

    /**
     * @brief шаг 4 комплектации: снять count единиц товара с паллеты 
     * хранения на столе на паллету отправки для заказа 
     */
    void takeUnits(const StorageAddress& address, int count);

    /**
     * @brief шаги 6-7 комплектации: вернуть паллету со стола на 
     * ее место на полке (зона упаковки -> зона хранения)
     */
    void returnPallet(const StorageAddress& address);

    // машина отправки уехала: ее груз "в упаковке" -> "отправлено"
    void registerShipped(const std::map<ProductType, int>& cargo);
private:
    int terminalCount_; // к-во терминалов системы учета (от 1 до 10)

    // место под паллеты
    struct StoragePlace {
        bool occupied = false; // на месте стоит паллета?
        ProductType type = ProductType::Light; // какой товар на паллете (occupied=true)
        int count = 0; // сколько единиц уже лежит
        int reserved = 0; // сколько единиц обещано (адрес выдан), но еще не положено
        bool onTable = false; // паллета увезена на стол комплектации зоны упаковки 
    };


    struct Shelf {
        std::array<StoragePlace, limits::kPalletPlacesPerShelf> places;
    };

    struct Rack {
        std::vector<Shelf> shelves;
    };

    std::vector<Rack> racks_;

    // счетчики по каждому виду товара
    std::map<ProductType, ProductStats> stats_;

    // сколько единиц каждого вида лежит на паллетах хранения 
    // и еще НЕ обещано ни одному заказу
    std::map<ProductType, int> freeForOrders_;

    mutable std::mutex mutex_;
};
