#include <cmath>
#include <stdexcept>

#include <BankRounding.hpp>

double bankRound(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("Для банковского округления требуется конечное число");
    }

    if (value == 0.0) {
        return 0.0;
    }

    double absolute = std::abs(value); // модуль
    double whole = std::floor(absolute); // округление вниз до ближайшего целого = целая_часть
    double fraction = absolute - whole; // дробная_часть

    double rounded = whole;
    // дробная_часть > 0.5 || дробная_часть = 0.5 && целая_часть нечетная
    if (fraction > 0.5 || (fraction == 0.5 && std::fmod(whole, 2.0) != 0.0)) {
        rounded += 1.0;
    }

    return std::copysign(rounded, value);
}
