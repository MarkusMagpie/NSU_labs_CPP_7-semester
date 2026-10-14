#pragma once

#include <ostream>
#include <random>
#include <vector>

#include <model/Pallet.h>

// пришедшая в ЗОНУ ПРИЕМКИ машина с товаром
struct Truck {
    int id = 0;
    std::vector<Pallet> pallets;

    /**
     * @brief Создает машину со случайным составом
     *
     * В машине от 10 до 100 паллет. 
     * На каждой паллете случайный вид товара
     * и от 1 до palletCapacity единиц.
     *
     * @param id номер машины (для вывода в лог)
     * @param gen генератор псевдослучайных чисел
     * @return новая машина
     */
    static Truck generate(int id, std::mt19937& gen);
};

std::ostream& operator<<(std::ostream& os, const Truck& truck);
