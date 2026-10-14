#pragma once

#include <ostream>

// адрес места под паллету в ЗОННЕ ХРАНЕНИЯ
struct StorageAddress {
    int rack = 0; // номер стеллажа
    int shelf = 0; // номер полки на стеллаже
    int place = 0; // номер места на полке (от 0 до 9)
};

std::ostream& operator<<(std::ostream& os, const StorageAddress& address);
