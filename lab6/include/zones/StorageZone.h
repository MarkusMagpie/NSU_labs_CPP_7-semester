#pragma once

#include <vector>

// ЗОНА ХРАНЕНИЯ: стеллажи -> полки -> места под паллеты
class StorageZone {
private:
    std::vector<int> shelvesPerRack_; // shelvesPerRack_[i] - к-во полок у стеллажа i (от 1 до 6)
    int terminalCount_; // к-во терминалов системы учета (от 1 до 10)
public:
    StorageZone(int rackCount, int terminalCount);

    int rackCount() const;
    int terminalCount() const;
    int totalPlaces() const;
};
