#pragma once

#include <ostream>
#include <random>

enum class ProductType {
    Light, // до 100 единиц на паллете
    Medium, //  до 30 единиц на паллете
    Heavy // до 4 единиц на паллете
};

inline ProductType kAllProductTypes[] = {ProductType::Light, ProductType::Medium, ProductType::Heavy};
inline int kProductTypesCount = 3;

/**
 * @brief Сколько единиц товара помещается на одну паллету данного вида
 *
 * @param type вид товара
 * @return 100 для легкого, 30 для среднего, 4 для тяжелого
 */
int palletCapacity(ProductType type);

/**
 * @brief Выбирает случайный вид товара
 *
 * @param gen генератор псевдослучайных чисел
 * @return один из трех видов товара
 */
ProductType randomProductType(std::mt19937& gen);

std::ostream& operator<<(std::ostream& os, ProductType type);
