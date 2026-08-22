#include <iostream>
#include <string>
#include "regex_engine.hpp"

int main() {
    std::string patrn;
    std::cout << "введи шаблон регулярного выражения: ";
    std::getline(std::cin, patrn);

    Regex regex(patrn);

    std::string tested_string;
    while (std::getline(std::cin, tested_string)) {
        bool regex_res = regex.match(tested_string);
        std::string res;
        res = regex_res
                  ? "ДА. Введенная строка соответствует данному ранее шаблону регулярного выражения"
                  : "НЕТ. Введенная строка НЕ соответствует данному ранее шаблону регулярного выражения";
        std::cout << res << "\n";
    }
    
    return 0;
}