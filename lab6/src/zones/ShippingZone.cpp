#include <algorithm>
#include <iostream>

#include <zones/ShippingZone.h>

ShippingZone::ShippingZone(int slotCount) : slots_(slotCount) {
    std::cout << "Зона отправки создана. В ней " << slotCount << " пока свободных слотов\n";
}

int ShippingZone::slotCount() const {
    return static_cast<int>(slots_.size());
}

int ShippingZone::arrive(const ShippingTruck& truck, std::stop_token stopToken, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_); // mutex.lock()

    // свободный слот = пустой optional
    auto isFree = [](std::optional<ShippingTruck>& slot) { 
        return !slot.has_value(); 
    };

    // жду появления свободного слота (аналог ReceivingDock::occupySlot)
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
        int slot = static_cast<int>(std::find_if(slots_.begin(), slots_.end(), isFree) - slots_.begin());
        slots_[slot] = truck;

        return slot;
    }
} // mutex.unlock()

void ShippingZone::depart(int slot) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        slots_[slot].reset(); // optional снова пустой -> слот свободен
    }

    slotFreed_.notify_one(); // разбудить одного менеджера, ждущего свободный слот
}

int ShippingZone::busyCount() {
    std::lock_guard<std::mutex> lock(mutex_);

    return static_cast<int>(std::count_if(slots_.begin(), slots_.end(),
        [](std::optional<ShippingTruck>& slot) { 
            return slot.has_value(); 
        }));
}
