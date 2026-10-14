#pragma once

#include <map>
#include <ostream>
#include <random>

#include <model/ProductType.h>

// Заказ: куда везти и сколько единиц каждого вида товара нужно
struct Order {
    int id = 0;
    int city = 0; // номер города: от 0 до (число городов - 1)
    std::map<ProductType, int> items; // вид товара -> количество единиц

    /**
     * @brief Создает заказ со случайным городом и составом.
     *
     * @param id номер заказа
     * @param cities сколько всего городов
     * @param gen генератор псевдослучайных чисел
     * @return новый заказ
     */
    static Order generate(int id, int cities, std::mt19937& gen);
};

std::ostream& operator<<(std::ostream& os, const Order& order);
