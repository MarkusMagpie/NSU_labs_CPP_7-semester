#pragma once

#include <ostream>

#include <model/ProductType.h>

struct Pallet {
    ProductType type = ProductType::Light;
    int count = 0;
};

std::ostream& operator<<(std::ostream& os, const Pallet& pallet);
