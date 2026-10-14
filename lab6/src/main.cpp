#include <iostream>
#include <string_view>

#include <model/Order.h>
#include <model/StorageAddress.h>
#include <model/Truck.h>
#include <utils/Config.h>
#include <utils/RandomGenerator.h>

// проверка из ТЗ: от 10 до 100 паллет, на каждой от 1 до palletCapacity единиц
bool isTruckValid(const Truck& truck) {
    int palletCount = static_cast<int>(truck.pallets.size());
    if (palletCount < limits::kMinTruckPallets || palletCount > limits::kMaxTruckPallets) {
        return false;
    }
    
    for (const Pallet& pallet : truck.pallets) {
        if (pallet.count < 1 || pallet.count > palletCapacity(pallet.type)) {
            return false;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    bool fast_mod = argc > 1 && std::string_view(argv[1]) == "--fast";

    Config config = Config::generate(fast_mod);
    std::cout << config << '\n';

    auto gen = RandomGenerator::getGenerator();

    Truck truck = Truck::generate(1, gen);
    std::cout << truck;
    std::cout << "Машина укомплектована корректно: " << (isTruckValid(truck) ? "да" : "НЕТ") << "\n\n";

    Order order = Order::generate(1, config.cities, gen);
    std::cout << order;

    StorageAddress address{3, 2, 7};
    std::cout << "Пример адреса палетты: " << address << '\n';

    return 0;
}
