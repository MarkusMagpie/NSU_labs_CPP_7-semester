#pragma once

enum class AccountType {
    Debit,
    Deposit,
    Credit
};

enum class Currency {
    RUB,
    YUAN,
    USD,
    EUR
};

// строки входных данных -> перечисления
AccountType parseAccountType(const char* text);
Currency parseCurrency(const char* text);

// перечисления -> строки
const char* accountTypeToString(AccountType type);
const char* currencyToString(Currency currency);
