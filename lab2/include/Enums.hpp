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

enum class DepositType {
    DailyRemaining,
    DailyMin,
    MonthlyRemaining,
    MonthlyMin,
    QuarterlyRemaining,
    QuarterlyMin,
    SemiAnnuallyRemaining,
    SemiAnnuallyMin,
    AnnuallyRemaining,
    AnnuallyMin
};

// строки входных данных -> перечисления
AccountType parseAccountType(const char* text);
Currency parseCurrency(const char* text);
DepositType parseDepositType(const char* text);

// перечисления -> строки
const char* accountTypeToString(AccountType type);
const char* currencyToString(Currency currency);
const char* depositTypeToString(DepositType type);
