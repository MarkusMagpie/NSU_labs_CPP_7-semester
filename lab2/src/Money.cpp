#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>

#include <Money.hpp>

MoneyParts moneyToParts(Money amount) {
    if (!std::isfinite(amount)) {
        throw std::invalid_argument("Денежная сумма должна быть конечной");
    }

    double absolute = std::abs(amount);
    /*
    std::modf разбивает число с плавающей точкой absolute на целую и дробную части; 
        целая часть запишется в wholeValue, дробная в fraction
    */
    double wholeValue;
    double fraction = std::modf(absolute, &wholeValue);

    // поместится ли целая часть в unsigned long long?
    // std::numeric_limits<unsigned long long>::digits = 64 (количество бит используемых для представления значения беззнакового типа)
    // std::ldexp(1.0, 64) = 1.0 * 2^64 - верхняя граница ull
    double limit = std::ldexp(1.0, std::numeric_limits<unsigned long long>::digits);
    if (wholeValue >= limit) {
        throw std::overflow_error("Целая часть денежной суммы слишком велика");
    }

    // double -> ull (wholeValue=12.0 -> whole=12)
    auto whole = static_cast<unsigned long long>(wholeValue);

    auto thousandths = static_cast<unsigned long long>(std::round(fraction * 1000.0));
    if (thousandths == 1000) {
        if (whole == std::numeric_limits<unsigned long long>::max()) {
            throw std::overflow_error("Целая часть денежной суммы слишком велика");
        }
        ++whole;
        thousandths = 0;
    }

    // после разложения на целую часть и тысячные получил то же значение double, которое было на входе?
    double reconstructed = static_cast<double>(whole) + static_cast<double>(thousandths) / 1000.0;
    if (reconstructed != absolute) {
        throw std::invalid_argument("Денежная сумма требует округления до тысячных");
    }

    return MoneyParts{amount < 0.0, whole, thousandths};
}

Money moneyFromParts(unsigned long long whole, unsigned long long thousandths, bool negative) {
    if (thousandths >= 1000) {
        throw std::invalid_argument("Количество тысячных должно быть от 0 до 999");
    }

    Money absolute = static_cast<double>(whole) + static_cast<double>(thousandths) / 1000.0;
    
    // не потерялись ли исходные части Money?
    MoneyParts recovered = moneyToParts(absolute);
    if (recovered.whole != whole || recovered.thousandths != thousandths) {
        throw std::overflow_error("Денежную сумму нельзя представить с требуемой точностью");
    }

    return negative && absolute != 0.0 ? -absolute : absolute;
}

void printMoney(Money amount) {
    const MoneyParts parts = moneyToParts(amount);
    std::printf("%s%llu.%03llu", parts.negative ? "-" : "", parts.whole, parts.thousandths);
}
