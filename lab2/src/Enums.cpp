#include <cstring>
#include <stdexcept>
#include <string>

#include <Enums.hpp>

AccountType parseAccountType(const char* text) {
    if (std::strncmp(text, "debit", 100) == 0) return AccountType::Debit;
    if (std::strncmp(text, "deposit", 100) == 0) return AccountType::Deposit;
    if (std::strncmp(text, "credit", 100) == 0) return AccountType::Credit;

    throw std::invalid_argument(std::string("Неизвестный тип счёта: ") + text);
}

Currency parseCurrency(const char* text) {
    if (std::strncmp(text, "RUB", 100) == 0) return Currency::RUB;
    if (std::strncmp(text, "YUAN", 100) == 0) return Currency::YUAN;
    if (std::strncmp(text, "USD", 100) == 0) return Currency::USD;
    if (std::strncmp(text, "EUR", 100) == 0) return Currency::EUR;

    throw std::invalid_argument(std::string("Неизвестная валюта: ") + text);
}

DepositType parseDepositType(const char* text) {
    if (std::strncmp(text, "Compounded Daily Remaining", 100) == 0) return DepositType::DailyRemaining;
    if (std::strncmp(text, "Compounded Daily Min", 100) == 0) return DepositType::DailyMin;
    if (std::strncmp(text, "Compounded Monthly Remaining", 100) == 0) return DepositType::MonthlyRemaining;
    if (std::strncmp(text, "Compounded Monthly Min", 100) == 0) return DepositType::MonthlyMin;
    if (std::strncmp(text, "Compounded Quarterly Remaining", 100) == 0) return DepositType::QuarterlyRemaining;
    if (std::strncmp(text, "Compounded Quarterly Min", 100) == 0) return DepositType::QuarterlyMin;
    if (std::strncmp(text, "Compounded Semi-Annually Remaining", 100) == 0) return DepositType::SemiAnnuallyRemaining;
    if (std::strncmp(text, "Compounded Semi-Annually Min", 100) == 0) return DepositType::SemiAnnuallyMin;
    if (std::strncmp(text, "Compounded Annually Remaining", 100) == 0) return DepositType::AnnuallyRemaining;
    if (std::strncmp(text, "Compounded Annually Min", 100) == 0) return DepositType::AnnuallyMin;

    throw std::invalid_argument(std::string("Неизвестный тип вклада: ") + text);
}

CreditType parseCreditType(const char* text) {
    if (std::strncmp(text, "Charged Daily", 100) == 0) return CreditType::Daily;
    if (std::strncmp(text, "Charged Monthly", 100) == 0) return CreditType::Monthly;
    if (std::strncmp(text, "Charged Quarterly", 100) == 0) return CreditType::Quarterly;
    if (std::strncmp(text, "Charged Semi-annually", 100) == 0) return CreditType::SemiAnnually;
    if (std::strncmp(text, "Charged Annually", 100) == 0) return CreditType::Annually;

    throw std::invalid_argument(std::string("Неизвестный тип кредита: ") + text);
}



const char* accountTypeToString(AccountType type) {
    switch (type) {
        case AccountType::Debit: return "debit";
        case AccountType::Deposit: return "deposit";
        case AccountType::Credit: return "credit";
    }

    throw std::invalid_argument("Неизвестное значение типа счёта");
}

const char* currencyToString(Currency currency) {
    switch (currency) {
        case Currency::RUB: return "RUB";
        case Currency::YUAN: return "YUAN";
        case Currency::USD: return "USD";
        case Currency::EUR: return "EUR";
    }

    throw std::invalid_argument("Неизвестное значение валюты");
}

const char* depositTypeToString(DepositType type) {
    switch (type) {
        case DepositType::DailyRemaining: return "Compounded Daily Remaining";
        case DepositType::DailyMin: return "Compounded Daily Min";
        case DepositType::MonthlyRemaining: return "Compounded Monthly Remaining";
        case DepositType::MonthlyMin: return "Compounded Monthly Min";
        case DepositType::QuarterlyRemaining: return "Compounded Quarterly Remaining";
        case DepositType::QuarterlyMin: return "Compounded Quarterly Min";
        case DepositType::SemiAnnuallyRemaining: return "Compounded Semi-Annually Remaining";
        case DepositType::SemiAnnuallyMin: return "Compounded Semi-Annually Min";
        case DepositType::AnnuallyRemaining: return "Compounded Annually Remaining";
        case DepositType::AnnuallyMin: return "Compounded Annually Min";
    }

    throw std::invalid_argument("Неизвестное значение типа вклада");
}

const char* creditTypeToString(CreditType type) {
    switch (type) {
        case CreditType::Daily: return "Charged Daily";
        case CreditType::Monthly: return "Charged Monthly";
        case CreditType::Quarterly: return "Charged Quarterly";
        case CreditType::SemiAnnually: return "Charged Semi-annually";
        case CreditType::Annually: return "Charged Annually";
    }

    throw std::invalid_argument("Неизвестное значение типа кредита");
}
