#include <iostream>

#include <utils/Config.h>
#include <utils/RandomGenerator.h>
#include <zones/StorageZone.h>

using namespace limits;

StorageZone::StorageZone(int rackCount, int terminalCount) : terminalCount_(terminalCount) {
    auto gen = RandomGenerator::getGenerator();
    for (int i = 0; i < rackCount; ++i) {
        shelvesPerRack_.push_back(RandomGenerator::getRandom(gen, kMinShelvesPerRack, kMaxShelvesPerRack));
    }

    std::cout << "Зона хранения создана: " << rackCount << " стеллажей, "
        << totalPlaces() << " мест под паллеты, "
        << terminalCount_ << " терминалов\n";
}

int StorageZone::rackCount() const {
    return static_cast<int>(shelvesPerRack_.size());
}

int StorageZone::terminalCount() const {
    return terminalCount_;
}

// сколько всего мест под паллеты: сумма полок всех стеллажей * 10
int StorageZone::totalPlaces() const {
    int shelves = 0;
    for (int count : shelvesPerRack_) {
        shelves += count;
    }

    return shelves * kPalletPlacesPerShelf;
}
