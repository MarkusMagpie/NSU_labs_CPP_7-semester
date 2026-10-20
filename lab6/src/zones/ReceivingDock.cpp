#include <algorithm>
#include <iostream>

#include <zones/ReceivingDock.h>

ReceivingDock::ReceivingDock(int slotCount) : busy_(slotCount, false) {
    std::cout << "Зона приемки создана. В ней " << slotCount << " слотов\n";
}

int ReceivingDock::slotCount() const {
    return static_cast<int>(busy_.size());
}

int ReceivingDock::occupySlot(std::stop_token stopToken, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_); // mutex.lock()

    // жду появления свободного слота (аналог TaskQueue::pop)
    bool hasFree = slotFreed_.wait_for(lock, 
        stopToken, 
        timeout, 
        [this] { 
            return std::find(busy_.begin(), busy_.end(), false) != busy_.end();
    });

    if (!hasFree) {
        return -1;
    } else {
        // мьютекс захвачен: никто не займет этот слот между поиском и пометкой слота занятым
        int slot = static_cast<int>(std::find(busy_.begin(), busy_.end(), false) - busy_.begin());
        busy_[slot] = true;
        
        return slot;
    }
} // mutex.unlock()

void ReceivingDock::releaseSlot(int slot) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        busy_[slot] = false;
    }

    slotFreed_.notify_one(); // разбудить одного менеджера, ждущего свободный слот
}

int ReceivingDock::busyCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return static_cast<int>(std::count(busy_.begin(), busy_.end(), true));
}
