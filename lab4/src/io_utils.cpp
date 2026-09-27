#include "io_utils.hpp"
#include <istream>
#include <sstream>

namespace lab4 {
std::string read_all(std::istream& in) {
    std::string result;
    char c;
    while (in.get(c)) {
        result += c;
    }

    return result;
}
}  // namespace lab4
