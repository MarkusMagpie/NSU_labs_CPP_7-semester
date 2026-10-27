#include <algorithm>
#include <iostream>

#include <utils/Config.h>
#include <utils/RandomGenerator.h>
#include <zones/PackingZone.h>

using namespace limits;

PackingZone::PackingZone(int tableCount) {
    auto gen = RandomGenerator::getGenerator();
    for (int i = 0; i < tableCount; ++i) {
        Table table;
        table.sortSlots = RandomGenerator::getRandom(gen, kMinTableSortSlots, kMaxTableSortSlots);
        table.packSlots = kTablePackSlots;
        tables_.push_back(table);
    }

    std::cout << "Зона упаковки создана: " << tableCount << " столов комплектации\n";
    for (int i = 0; i < tableCount; ++i) {
        std::cout << "  стол " << i << ": " 
            << tables_[i].sortSlots << " слотов для разбора, " 
            << tables_[i].packSlots << " для упаковки\n";
    }
}

int PackingZone::tableCount() const {
    return static_cast<int>(tables_.size());
}

int PackingZone::bookTable(std::stop_token stopToken, 
    std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_); // mutex.lock

    auto isFree = [](const Table& table) { return !table.booked; };

    // жду свободный стол (ReceivingDock::occupySlot)
    bool hasFree = tableFreed_.wait_for(lock, 
        stopToken, 
        timeout, 
        [this, &isFree] {
            return std::find_if(tables_.begin(), tables_.end(), isFree) != tables_.end();
    });

    if (!hasFree) {
        // за timeout стол так и не освободился (или остановка по токену)
        return -1;
    } else {
        // мьютекс захвачен
        int table = static_cast<int>(std::find_if(tables_.begin(), tables_.end(), isFree) - tables_.begin());
        tables_[table].booked = true;
        
        return table;
    }
} // mutex.unlock

void PackingZone::releaseTable(int table) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tables_[table].booked = false;
    }

    tableFreed_.notify_one(); // разбудить одного грузчика ждущего стол
}

int PackingZone::sortSlots(int table) const {
    return tables_[table].sortSlots;
}

int PackingZone::packSlots(int table) const {
    return tables_[table].packSlots;
}

int PackingZone::busyCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return static_cast<int>(std::count_if(tables_.begin(), tables_.end(),
        [](const Table& table) { return table.booked; }));
}
