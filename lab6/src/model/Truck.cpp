#include <algorithm>

#include <model/Truck.h>
#include <utils/Config.h>
#include <utils/RandomGenerator.h>

using namespace limits;

Truck Truck::generate(int id, std::mt19937& gen) {
    Truck truck;
    truck.id = id;

    int palletCount = RandomGenerator::getRandom(gen, kMinTruckPallets, kMaxTruckPallets);
    for (int i = 0; i < palletCount; ++i) {
        Pallet pallet;
        pallet.type = randomProductType(gen);

        // каждая паллета имеет от 1 до 100 единиц товара, НО важен и тип паллеты, где максимум может быть меньше
        int maxItems = std::min(kMaxPalletItems, palletCapacity(pallet.type));
        pallet.count = RandomGenerator::getRandom(gen, kMinPalletItems, maxItems);
        truck.pallets.push_back(pallet);
    }

    return truck;
}

std::ostream& operator<<(std::ostream& os, const Truck& truck) {
    os << "машина #" << truck.id << " (" << truck.pallets.size() << " паллет):";
    
    for (const Pallet& pallet : truck.pallets) {
        os << "\n  " << pallet;
    }

    return os << '\n';
}
