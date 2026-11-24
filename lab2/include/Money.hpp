#pragma once

#include <BasicTypes.hpp>

// части суммы для вывода: знак и неотрицательные целая часть и тысячные
struct MoneyParts {
    bool negative; // 1-; 0+
    unsigned long long whole;
    unsigned long long thousandths;
};

// создать сумму из целой части и тысячных (0-999) | {negative, whole, thousandths} -> Money
// пример использования: Money amount = moneyFromParts(12, 5); // =12.005
Money moneyFromParts(unsigned long long whole, unsigned long long thousandths, bool negative = false);

// разобрать сумму на части НЕ округляя ее для банковских расчетов | {negative, whole, thousandths} <- Money
MoneyParts moneyToParts(Money amount);

void printMoney(Money amount);
