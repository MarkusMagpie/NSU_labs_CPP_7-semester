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
