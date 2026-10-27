#include <algorithm>
#include <iostream>
#include <sstream>

#include <utils/Config.h>
#include <utils/Logger.h>
#include <utils/RandomGenerator.h>
#include <zones/StorageZone.h>

using namespace limits;

StorageZone::StorageZone(int rackCount, int terminalCount) : terminalCount_(terminalCount) {
    auto gen = RandomGenerator::getGenerator();

    // у каждого стеллажа рандомное к-во полок; resize создает полки, а в каждой полке
    // сразу kPalletPlacesPerShelf=10 пустых мест (occupied = false, count = 0)
    for (int i = 0; i < rackCount; ++i) {
        Rack rack;
        rack.shelves.resize(RandomGenerator::getRandom(gen, kMinShelvesPerRack, kMaxShelvesPerRack));
        
        racks_.push_back(rack);
    }

    // нулевые счетчики для всех видов товара
    for (ProductType type : kAllProductTypes) {
        stats_[type] = ProductStats{};
        freeForOrders_[type] = 0;
    }

    std::cout << "Зона хранения создана: " << rackCount << " стеллажей, "
        << totalPlaces() << " мест под паллеты, "
        << terminalCount_ << " терминалов\n";
}

int StorageZone::rackCount() const {
    return static_cast<int>(racks_.size());
}

int StorageZone::terminalCount() const {
    return terminalCount_;
}

// сколько всего мест под паллеты: сумма полок всех стеллажей * 10
int StorageZone::totalPlaces() const {
    int shelves = 0;
    for (const Rack& rack : racks_) {
        shelves += static_cast<int>(rack.shelves.size());
    }

    return shelves * kPalletPlacesPerShelf;
}

std::vector<StorageZone::Placement> StorageZone::requestPlacement(ProductType type, int count) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    std::vector<Placement> result;
    int remaining = count; // сколько единиц еще не запросили адрес
    int capacity = palletCapacity(type); // 100 || 30 || 4

    // 1 дозаполнить уже начатые паллеты того же вида
    for (int r = 0; r < rackCount() && remaining > 0; ++r) {
        for (int s = 0; s < static_cast<int>(racks_[r].shelves.size()) && remaining > 0; ++s) {
            for (int p = 0; p < kPalletPlacesPerShelf && remaining > 0; ++p) {
                StoragePlace& place = racks_[r].shelves[s].places[p];
                
                // пустое место или паллета с другим товаром
                if (!place.occupied || place.type != type) {
                    continue;
                }
                // паллета сейчас на столе комплектации -> класть на нее товар нельзя
                if (place.onTable) {
                    continue;
                }

                // свободно = лимит - уже лежит - уже обещано другим грузчикам
                int free = capacity - place.count - place.reserved;
                if (free <= 0) {
                    continue; // паллета полная или полностью забронирована
                }

                int reserve_amount = std::min(free, remaining);
                place.reserved += reserve_amount; // бронь: до confirmPlacement() эту емкость никто не получит
                remaining -= reserve_amount;
                
                result.push_back({StorageAddress{r, s, p}, reserve_amount});
            }
        }
    }

    // 2 остаток - на пустые места, новой паллетой
    for (int r = 0; r < rackCount() && remaining > 0; ++r) {
        for (int s = 0; s < static_cast<int>(racks_[r].shelves.size()) && remaining > 0; ++s) {
            for (int p = 0; p < kPalletPlacesPerShelf && remaining > 0; ++p) {
                StoragePlace& place = racks_[r].shelves[s].places[p];
                if (place.occupied) {
                    continue;
                }

                // занятие места чтобы другой грузчик не поставил сюда свою паллету
                place.occupied = true;
                place.type = type;

                int reserve_amount = std::min(capacity, remaining);
                place.reserved = reserve_amount;
                remaining -= reserve_amount;

                result.push_back({StorageAddress{r, s, p}, reserve_amount});
            }
        }
    }

    // если склад заполнен, а remaining > 0?

    return result;
} // mutex.unlock()

void StorageZone::confirmPlacement(Placement& placement) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()
    
    const StorageAddress& address = placement.address;
    StoragePlace& place = racks_[address.rack].shelves[address.shelf].places[address.place];
    place.reserved -= placement.count;
    place.count += placement.count;

    // для задачи инвентаризации
    stats_[place.type].inStorageZone += placement.count;
    
    // новый товар на полке не обещан заказам -> можно резервировать
    freeForOrders_[place.type] += placement.count;
} // mutex.unlock()

void StorageZone::registerReceived(ProductType type, int count) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    stats_[type].receivedInReceivingDock += count;
} // mutex.unlock()

StorageZone::ProductStats StorageZone::stats(ProductType type) const {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    return stats_.at(type);
} // mutex.unlock()

StorageZone::ProductReport StorageZone::report(ProductType type) const {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()
    
    ProductReport result;
    result.type = type;
    result.stats = stats_.at(type);
    
    for (int r = 0; r < rackCount(); ++r) {
        for (int s = 0; s < static_cast<int>(racks_[r].shelves.size()); ++s) {
            for (int p = 0; p < kPalletPlacesPerShelf; ++p) {
                const StoragePlace& place = racks_[r].shelves[s].places[p];
                // onTable: паллета на столе комплектации - ее товар уже "в упаковке", а не в зоне хранения
                if (place.occupied && !place.onTable && place.type == type && place.count > 0) {
                    result.pallets.push_back({StorageAddress{r, s, p}, place.count});
                }
            }
        }
    }

    return result;
} // mutex.unlock()

bool StorageZone::reserveForOrder(const std::map<ProductType, int>& items) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    // 1 проверить все виды
    for (auto& [type, count] : items) {
        if (freeForOrders_.at(type) < count) {
            // товара не хватает -> заказ сейчас не ставлю
            return false;
        }
    }

    // 2 хватает по всем видам -> закрепить товар за заказом
    for (auto& [type, count] : items) {
        freeForOrders_.at(type) -= count;
    }

    return true;
} // mutex.unlock()

void StorageZone::releaseReservation(const std::map<ProductType, int>& items) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    // обещанный несобранному заказу товар снова доступен другим заказам
    for (auto& [type, count] : items) {
        freeForOrders_.at(type) += count;
    }
} // mutex.unlock()



std::optional<StorageZone::CheckedOutPallet> StorageZone::checkoutPallet(ProductType type) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    for (int r = 0; r < rackCount(); ++r) {
        for (int s = 0; s < static_cast<int>(racks_[r].shelves.size()); ++s) {
            for (int p = 0; p < kPalletPlacesPerShelf; ++p) {
                StoragePlace& place = racks_[r].shelves[s].places[p];

                // стоит на полке (не на другом столе), 
                // нужного вида, 
                // с товаром,
                // без брони разгрузки (reserved == 0) 
                //  - иначе грузчик-разгрузчик понес бы товар
                // на паллету которой уже нет на полке
                if (!place.occupied || place.onTable || place.type != type
                    || place.count == 0 || place.reserved != 0) {
                    continue;
                }

                place.onTable = true;

                // ТЗ "паллета перемещена в зону упаковки": 3a; 3b
                stats_[type].inStorageZone -= place.count;
                stats_[type].inPackingZone += place.count;

                return CheckedOutPallet{StorageAddress{r, s, p}, type, place.count};
            }
        }
    }

    return std::nullopt;
} // mutex.unlock()

void StorageZone::takeUnits(const StorageAddress& address, int count) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    StoragePlace& place = racks_[address.rack].shelves[address.shelf].places[address.place];
    if (!place.onTable || count > place.count) {
        Logger::log("ОШИБКА takeUnits: паллета не на столе или на ней меньше " + std::to_string(count) + " шт");
        return;
    }

    // единицы переложены с паллеты хранения на паллету отправки
    place.count -= count;
} // mutex.unlock()

void StorageZone::returnPallet(const StorageAddress& address) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    StoragePlace& place = racks_[address.rack].shelves[address.shelf].places[address.place];
    if (!place.onTable) {
        Logger::log("ОШИБКА returnPallet: паллета и так на полке");
        return;
    }

    place.onTable = false;

    // оставшийся на паллете товар возвращается в зону хранения
    stats_[place.type].inPackingZone -= place.count;
    stats_[place.type].inStorageZone += place.count;

    // шаг 6 ТЗ: "при снятии последней единицы товара с паллеты слот под паллет на полке освобождается"
    if (place.count == 0) {
        place.occupied = false;
    }
} // mutex.unlock()

void StorageZone::registerShipped(const std::map<ProductType, int>& cargo) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()

    for (auto& [type, count] : cargo) {
        stats_[type].inPackingZone -= count;
        stats_[type].shipped += count;
    }
} // mutex.unlock()
