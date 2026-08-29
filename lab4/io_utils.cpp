#include "io_utils.hpp"
#include <istream>
#include <sstream>

namespace lab4 {

// std::istream - полиморфизм. можно передавать разные потоки ввода, например std::sin, std::istringstream (в тестах)
std::string read_all(std::istream& in) {
    std::string result;
    char c;
    while (in.get(c)) {
        result += c;
    }

    return result;
}

}  // namespace lab4
