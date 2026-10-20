#pragma once

#include <array>
#include <vector>
#include <map>
#include <mutex>

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
        int receivedInReceivingDock = 0;    // принято в зоне приемки (накопительно, только растет)
        int inStorageZone = 0;      // лежит в зоне хранения
        int inPackingZone = 0;      // в процессе упаковки
        int shipped = 0;            // отправлено (накопительно, только растет)
    };

    // действие 2 приемки: "ввести вид и количество -> получить адрес"
    // На терминале системы учета ввести количество и вид товара на паллете и получить адрес (или адреса) хранения товара в зоне хранения
    std::vector<Placement> requestPlacement(ProductType type, int count);

    // действие 4: "отметить что товар размещен"
    // Отметить в терминале системы учета в зоне хранения, что товар был размещен по заданному адресу
    void confirmPlacement(Placement& placement);

    // счетчики учета по одному виду товара
    ProductStats stats(ProductType type) const;
private:
    int terminalCount_; // к-во терминалов системы учета (от 1 до 10)

    // место под паллеты
    struct StoragePlace {
        bool occupied = false; // на месте стоит паллета?
        ProductType type = ProductType::Light; // какой товар на паллете (occupied=true)
        int count = 0; // сколько единиц уже лежит
        int reserved = 0; // сколько единиц обещано (адрес выдан), но еще не положено
    };


    struct Shelf {
        std::array<StoragePlace, limits::kPalletPlacesPerShelf> places;
    };

    struct Rack {
        std::vector<Shelf> shelves;
    };

    std::vector<Rack> racks_;

    // счетчики по каждому виду товара; поле закрыто: менять их может только сама зона
    std::map<ProductType, ProductStats> stats_;

    mutable std::mutex mutex_;
};
