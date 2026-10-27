#include <algorithm>
#include <iostream>
#include <mutex>
#include <thread>

#include <utils/Config.h>
#include <utils/RandomGenerator.h>
#include <zones/ShippingZone.h>

ShippingZone::ShippingZone(int slotCount) 
    : slots_(slotCount), doors_(slotCount) {
    std::cout << "Зона отправки создана. В ней " << slotCount << " пока свободных слотов\n";
}

int ShippingZone::slotCount() const {
    return static_cast<int>(slots_.size());
}

int ShippingZone::arrive(const ShippingTruck& truck, std::stop_token stopToken, std::chrono::milliseconds timeout) {
    int slot = -1;
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        // свободный слот = пустой optional
        auto isFree = [](std::optional<ShippingTruck>& slot) {
            return !slot.has_value();
        };

        // менеджер ждет появления свободного слота
        bool hasFree = slotFreed_.wait_for(lock, 
            stopToken, 
            timeout, 
            [this, &isFree] {
                return std::find_if(slots_.begin(), slots_.end(), isFree) != slots_.end();
        });

        if (!hasFree) {
            return -1;
        } else {
            // мьютекс захвачен -> никто не займет этот слот между поиском и постановкой машины
            slot = static_cast<int>(std::find_if(slots_.begin(), slots_.end(), isFree) - slots_.begin());
            slots_[slot] = truck;
        } 
    }

    boardChanged_.notify_all();
    
    return slot;
}

void ShippingZone::depart(int slot) {
    {
        // сначала дверь: машину нельзя убрать, пока внутри идет погрузка
        std::lock_guard<std::mutex> door(doors_[slot]);
        std::unique_lock<std::shared_mutex> lock(mutex_);
        slots_[slot].reset(); // optional снова пустой -> слот свободен
    }

    slotFreed_.notify_one(); // разбудить одного менеджера ждущего свободный слот

    boardChanged_.notify_all(); // оповестить грузчиков об уходе машины
}

ShippingZone::LoadResult ShippingZone::load(int slot, 
    int truckId, 
    const Pallet& pallet, 
    std::chrono::milliseconds actionTime) 
{
    LoadResult result;
    bool departed = false;
    {
        // зайти в машину: если внутри другой грузчик то надо ждать своей очереди у двери
        std::lock_guard<std::mutex> door(doors_[slot]);

        // перепроверка
        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            const std::optional<ShippingTruck>& truck = slots_[slot];
            
            if (!truck || truck->id != truckId || truck->loaded >= truck->capacity) {
                return result; // повторить попытку
            }
        }

        std::this_thread::sleep_for(actionTime);

        // отметка погрузки на терминале у слота
        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            ShippingTruck& truck = *slots_[slot];
            ++truck.loaded;
            truck.cargo[pallet.type] += pallet.count;

            // "машина отправляется, когда она полностью загружена"
            if (truck.loaded == truck.capacity) {
                result.truck = truck;
                slots_[slot] = std::nullopt;
                departed = true;
            }
        }
    }

    if (departed) {
        result.status = LoadStatus::Departed;
        slotFreed_.notify_one(); // менеджер может поставить новую машину
        boardChanged_.notify_all(); // оповестить грузчиков об уходе машины
    } else {
        result.status = LoadStatus::Loaded;
    }

    return result;
}

int ShippingZone::busyCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    return static_cast<int>(std::count_if(slots_.begin(), slots_.end(),
        [](const std::optional<ShippingTruck>& s) {
            return s.has_value();
        }));
}

std::optional<ShippingZone::TruckRef> ShippingZone::findTruckLocked(int city) const {
    for (int slot = 0; slot < slotCount(); ++slot) {
        const std::optional<ShippingTruck>& truck = slots_[slot];
        if (truck && truck->city == city && truck->loaded < truck->capacity) {
            return TruckRef{slot, truck->id};
        }
    }

    return std::nullopt;
}

std::optional<ShippingZone::TruckRef> ShippingZone::findTruck(int city) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    return findTruckLocked(city);
}

std::optional<ShippingZone::TruckRef> ShippingZone::waitForTruck(int city, 
    std::stop_token stopToken, 
    std::chrono::milliseconds timeout) 
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::optional<TruckRef> found;
    // грузчик ждет прихода в нужный ему город машины со свободным местом
    boardChanged_.wait_for(lock, 
        stopToken, 
        timeout, 
        [this, city, &found] {
            found = findTruckLocked(city);
            return found.has_value();
    });

    return found;
}

ShippingZone::ShipResult ShippingZone::ship(int city, 
    const Pallet& pallet, 
    std::chrono::milliseconds actionTime, 
    std::chrono::milliseconds timeUnit, 
    std::stop_token stopToken) {
    ShipResult result;

    for (int attempt = 1; attempt <= limits::kMaxShippingAttempts && !stopToken.stop_requested(); ++attempt) {
        result.attempts = attempt;

        // 1 просмотреть доску занятости, найти свободную машину в нужный город
        std::optional<TruckRef> truck = findTruck(city);

        // 3a машины в нужный город нет -> ждать от 1 до 5 единиц пока не появится
        if (!truck) {
            int units = RandomGenerator::getRandom(limits::kMinActionTime, limits::kMaxActionTime);
            truck = waitForTruck(city, stopToken, timeUnit * units);
            if (!truck) {
                continue; // 3b следующая попытка тк не дождался
            }
        }

        // 2 машина есть -> встать в очередь к двери и погрузить
        LoadResult loaded = load(truck->slot, truck->truckId, pallet, actionTime);
        if (loaded.status == LoadStatus::TruckGone) {
            continue; // 2ai машина наполнилась и уехала пока шел -> повторить попытку
        }

        result.shipped = true;
        if (loaded.status == LoadStatus::Departed) {
            result.departedTruck = loaded.truck;
        }

        return result;
    }

    // 3c после 5 неудачных попыток вызывающий возвращает паллету на стол комплектации
    return result;
}

ShippingZone::Board ShippingZone::board() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    return slots_; // копия: после выхода из метода другие потоки могут менять настоящую доску
}
