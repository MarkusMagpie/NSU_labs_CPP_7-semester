// компилиоровать из папки lab4
// g++ -std=c++17 -Iinclude examples/variant_demo.cpp -o res && ./res

#include <iostream>
#include <string>

#include "CustomVariant.hpp"

using Variant = lab4::CustomVariant<int, std::string>;

int main() {
    Variant a(42);
    std::cout << "a: index=" << a.index() << " value=" << a.get<int>() << "\n";

    Variant b(std::string("hello"));
    std::cout << "b: index=" << b.index() << " value=" << b.get<std::string>() << "\n";

    Variant c = b; // копирование
    std::cout << "c: index=" << c.index() << " value=" << c.get<std::string>() << "\n";

    // Variant d(true);


    std::cout << "конец main\n";
}