#pragma once

#include <BasicTypes.hpp>
#include <Enums.hpp>

struct Account {
    unsigned long long id; // Уникальный номер счета
    AccountType type; // Тип счета: debit; deposit; credit
    Money balance; // Объем средств на счете
    Currency currency; // Валюта счета: RUB; YUAN; USD; EUR
};
