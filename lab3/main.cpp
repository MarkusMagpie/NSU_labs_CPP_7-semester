#include <iostream>
#include <string>
#include "regex_engine.hpp"

int main() {
    std::string patrn;
    std::cout << "Введите шаблон регулярного выражения: ";
    std::getline(std::cin, patrn);

    Regex regex(patrn);

    std::string line;
    while (std::getline(std::cin, line)) {
        bool regex_res = regex.match(line);
        std::string res;
        if (regex_res) {
            res = "Введенная строка соответствует регулярному выражению";
        } else {
            res = "Введенная строка не соответствует регулярному выражению";
        }
        std::cout << res << "\n";
    }
    
    return 0;
}