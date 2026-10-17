#include <utils/Config.h>
#include <utils/RandomGenerator.h>

using namespace limits;

Config Config::generate(bool fastMode) {
    auto gen = RandomGenerator::getGenerator();
    auto rnd = [&gen](int min, int max) { 
        return RandomGenerator::getRandom(gen, min, max); 
    };

    Config config;
    config.fastMode = fastMode;
    config.receivingSlots = rnd(kMinReceivingSlots, kMaxReceivingSlots);
    config.shippingSlots = rnd(kMinShippingSlots, kMaxShippingSlots);
    config.storageRacks = rnd(kMinRacks, kMaxRacks);
    config.storageTerminals = rnd(kMinStorageTerminals, kMaxStorageTerminals);
    config.packingTables = rnd(kMinPackingTables, kMaxPackingTables);
    config.cities = rnd(kMinCities, kMaxCities);
    config.loaders = rnd(kMinLoaders, kMaxLoaders);
    config.managers = rnd(kMinManagers, kMaxManagers);

    return config;
}

std::chrono::milliseconds Config::timeUnit() {
    if (fastMode) {
        return std::chrono::milliseconds(1);
    }
    
    return std::chrono::milliseconds(1000);
}

std::ostream& operator<<(std::ostream& os, const Config& config) {
    os << "Конфигурация склада:\n"
        << " Режим: " << (config.fastMode ? "быстрый (мс)" : "обычный (с)") << '\n'
        << " Слотов приемки: " << config.receivingSlots << '\n'
        << " Слотов отправки: " << config.shippingSlots << '\n'
        << " Стеллажей: " << config.storageRacks << '\n'
        << " Терминалов хранения: " << config.storageTerminals << '\n'
        << " Столов комплектации: " << config.packingTables << '\n'
        << " Городов: " << config.cities << '\n'
        << " Грузчиков: " << config.loaders << '\n'
        << " Менеджеров: " << config.managers << '\n';

    return os;
}
