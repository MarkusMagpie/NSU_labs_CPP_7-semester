#include <model/ProductType.h>
#include <utils/RandomGenerator.h>

int palletCapacity(ProductType type) {
    switch (type) {
        case ProductType::Light: return 100;
        case ProductType::Medium: return 30;
        case ProductType::Heavy: return 4;
    }

    return 0;
}

ProductType randomProductType(std::mt19937& gen) {
    int index = RandomGenerator::getRandom(gen, 0, kProductTypesCount - 1);
    
    return kAllProductTypes[index];
}

std::ostream& operator<<(std::ostream& os, ProductType type) {
    switch (type) {
        case ProductType::Light: return os << "Легкий";
        case ProductType::Medium: return os << "Средний";
        case ProductType::Heavy: return os << "Тяжелый";
    }
    
    return os;
}
