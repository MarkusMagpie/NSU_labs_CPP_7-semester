#pragma once

#include <vector>

// ЗОНА УПАКОВКИ: столы комплектации
class PackingZone {
private:
    // Каждый стол имеет от 1 до 3 слотов под паллеты для разбора и 2 слота для упаковки (limits::kTablePackSlots)
    std::vector<int> sortSlotsPerTable_; // слоты для разбора
public:
    PackingZone(int tableCount);

    int tableCount() const;
};
