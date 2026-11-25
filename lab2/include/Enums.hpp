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

enum class CreditType {
    Daily,
    Monthly,
    Quarterly,
    SemiAnnually,
    Annually
};

enum class ClientType {
    IndividualClient,
    LegalEntity,
    VipIndividualClient,
    VipLegalEntity,
    NotAClient
};

enum class WorkPlaceType {
    ClientManager,
    CashDesk,
    CurrencyExchange,
    VipClientManager
};

// строки входных данных -> перечисления
AccountType parseAccountType(const char* text);
Currency parseCurrency(const char* text);
DepositType parseDepositType(const char* text);
CreditType parseCreditType(const char* text);
ClientType parseClientType(const char* text);
WorkPlaceType parseWorkPlaceType(const char* text);

// перечисления -> строки
const char* accountTypeToString(AccountType type);
const char* currencyToString(Currency currency);
const char* depositTypeToString(DepositType type);
const char* creditTypeToString(CreditType type);
const char* clientTypeToString(ClientType type);
const char* workPlaceTypeToString(WorkPlaceType type);
