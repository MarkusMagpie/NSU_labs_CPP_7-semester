#include <iostream>

#include <utils/Config.h>
#include <utils/RandomGenerator.h>
#include <zones/PackingZone.h>

using namespace limits;

PackingZone::PackingZone(int tableCount) {
    auto gen = RandomGenerator::getGenerator();
    for (int i = 0; i < tableCount; ++i) {
        sortSlotsPerTable_.push_back(RandomGenerator::getRandom(gen, kMinTableSortSlots, kMaxTableSortSlots));
    }

    std::cout << "Зона упаковки создана: " << tableCount << " столов комплектации\n";
    for (int i = 0; i < tableCount; ++i) {
        std::cout << "  стол " << i << ": " << sortSlotsPerTable_[i] << " слотов для разбора, "
                << kTablePackSlots << " для упаковки\n";
    }
}

int PackingZone::tableCount() const {
    return static_cast<int>(sortSlotsPerTable_.size());
}
