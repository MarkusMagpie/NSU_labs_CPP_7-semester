#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <InputReader.hpp>
#include <Money.hpp>

namespace {
// scanf допускает пробелы в строковых полях - пробелы перед '#'
void trimTrailingSpaces(char* text) {
    std::size_t length = std::strlen(text);

    while (length > 0 && text[length - 1] == ' ') {
        text[--length] = '\0';
    }
}
} // namespace

void readAccounts(AccountTable& accounts) {
    if (accounts.size() != 0) {
        throw std::invalid_argument("Для чтения Accounts требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Accounts %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Accounts");
    }
    if (count > AccountTable::capacity) {
        throw std::runtime_error("Количество счетов превышает вместимость таблицы счетов Accounts");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        Account account{};
        char typeText[100]{};
        char currencyText[100]{};
        unsigned long long whole = 0;
        unsigned long long thousandths = 0;
        // пример читаемой строки: 1 # debit # 1250.555 # RUB
        const int fields = std::scanf(
            "%llu # %99[a-zA-Z0-9/_ ] # %llu.%llu # %99[a-zA-Z0-9/_ ]",
            &account.id, typeText, &whole, &thousandths, currencyText
        );
        if (fields != 5) {
            throw std::runtime_error("Не удалось прочитать запись Accounts");
        }

        // избавление от пробелов справа. Например: "debit  " -> "debit " -> "debit"
        trimTrailingSpaces(typeText);
        trimTrailingSpaces(currencyText);

        account.type = parseAccountType(typeText);
        account.currency = parseCurrency(currencyText);
        account.balance = moneyFromParts(whole, thousandths);

        if (!accounts.add(account)) {
            throw std::runtime_error("Таблица счетов Accounts заполнена");
        }
    }
}

void readDeposits(DepositTable& deposits) {
    if (deposits.size() != 0) {
        throw std::invalid_argument("Для чтения Deposits требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Debits %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Debits"); // вообще Deposits, не Debits
    }
    if (count > DepositTable::capacity) {
        throw std::runtime_error("Количество вкладов превышает вместимость таблицы Deposits");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        Deposit deposit{};
        char typeText[100]{};
        const int fields = std::scanf(
            "%llu # %le # %99[a-zA-Z0-9/_ -] # %llu # %llu",
            &deposit.id, &deposit.annualRate, typeText, &deposit.creationDate, &deposit.termDays
        );
        if (fields != 5) {
            throw std::runtime_error("Не удалось прочитать запись Deposits");
        }

        trimTrailingSpaces(typeText);
        deposit.type = parseDepositType(typeText);

        if (!deposits.add(deposit)) {
            throw std::runtime_error("Таблица вкладов Deposits заполнена");
        }
    }
}

void readCredits(CreditTable& credits) {
    if (credits.size() != 0) {
        throw std::invalid_argument("Для чтения Credits требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Credits %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Credits");
    }
    if (count > CreditTable::capacity) {
        throw std::runtime_error("Количество кредитов превышает вместимость таблицы Credits");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        Credit credit{};
        unsigned long long whole = 0;
        unsigned long long thousandths = 0;
        char typeText[100]{};
        // “%llu # %le # %llu.%llu # %99[a-zA-Z0-9/_ ]\n”
        const int fields = std::scanf(
            "%llu # %le # %llu.%llu # %99[a-zA-Z0-9/_ -]",
            &credit.id, &credit.annualRate, &whole, &thousandths, typeText
        );
        if (fields != 5) {
            throw std::runtime_error("Не удалось прочитать запись Credits");
        }

        trimTrailingSpaces(typeText);
        credit.loanAmount = moneyFromParts(whole, thousandths);
        credit.type = parseCreditType(typeText);

        if (!credits.add(credit)) {
            throw std::runtime_error("Таблица кредитов Credits заполнена");
        }
    }
}

void readClients(ClientTable& clients) {
    if (clients.size() != 0) {
        throw std::invalid_argument("Для чтения Clients требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Clients %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Clients");
    }
    if (count > ClientTable::capacity) {
        throw std::runtime_error("Количество клиентов превышает вместимость таблицы Clients");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        Client client{};
        char typeText[100]{};
        // “%llu # %99[a-zA-Z0-9/_ ] # %99[a-zA-Z0-9/_ ]\n”
        const int fields = std::scanf(
            "%llu # %99[a-zA-Z0-9/_ ] # %99[a-zA-Z0-9/_ ]",
            &client.id, client.name, typeText
        );
        if (fields != 3) {
            throw std::runtime_error("Не удалось прочитать запись Clients");
        }

        trimTrailingSpaces(client.name);
        trimTrailingSpaces(typeText);
        client.type = parseClientType(typeText);
        
        if (!clients.add(client)) {
            throw std::runtime_error("Таблица клиентов Clients заполнена");
        }
    }
}

void readClientAccounts(ClientAccountTable& clientAccounts) {
    if (clientAccounts.size() != 0) {
        throw std::invalid_argument("Для чтения Client Debit требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Client Debit %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Client Debit");
    }
    if (count > ClientAccountTable::capacity) {
        throw std::runtime_error("Количество счетов клиентов превышает вместимость таблицы Client Debit");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        ClientAccount clientAccount{};
        // “%llu # %llu # %llu\n”
        const int fields = std::scanf(
            "%llu # %llu # %llu",
            &clientAccount.clientId, &clientAccount.accountId, &clientAccount.depositId
        );
        if (fields != 3) {
            throw std::runtime_error("Не удалось прочитать запись Client Debit");
        }

        if (!clientAccounts.add(clientAccount)) {
            throw std::runtime_error("Таблица счетов клиентов Client Debit заполнена");
        }
    }
}

void readBankAccounts(BankAccountTable& bankAccounts) {
    if (bankAccounts.size() != 0) {
        throw std::invalid_argument("Для чтения Bank Accounts требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Bank Accounts %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Bank Accounts");
    }
    if (count > BankAccountTable::capacity) {
        throw std::runtime_error("Количество счетов банка превышает вместимость таблицы Bank Accounts");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        BankAccount bankAccount{};
        if (std::scanf("%llu", &bankAccount.accountId) != 1) {
            throw std::runtime_error("Не удалось прочитать запись Bank Accounts");
        }

        if (!bankAccounts.add(bankAccount)) {
            throw std::runtime_error("Таблица счетов банка Bank Accounts заполнена");
        }
    }
}

void readClientCredits(ClientCreditTable& clientCredits) {
    if (clientCredits.size() != 0) {
        throw std::invalid_argument("Для чтения Client Credit требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Client Credit %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Client Credit");
    }
    if (count > ClientCreditTable::capacity) {
        throw std::runtime_error("Количество долговых обязательств превышает вместимость таблицы Client Credit");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        ClientCredit clientCredit{};
        const int fields = std::scanf(
            "%llu # %llu # %llu",
            &clientCredit.clientId, &clientCredit.accountId, &clientCredit.creditId
        );
        if (fields != 3) {
            throw std::runtime_error("Не удалось прочитать запись Client Credit");
        }

        if (!clientCredits.add(clientCredit)) {
            throw std::runtime_error("Таблица долговых обязательств Client Credit заполнена");
        }
    }
}

void readWorkPlaces(WorkPlaceTable& workPlaces) {
    if (workPlaces.size() != 0) {
        throw std::invalid_argument("Для чтения Work Places требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Work Places %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Work Places");
    }
    if (count > WorkPlaceTable::capacity) {
        throw std::runtime_error("Количество записей превышает вместимость таблицы Work Places");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        WorkPlace workPlace{};
        char typeText[100]{};
        const int fields = std::scanf(
            " %99[a-zA-Z0-9/_ ] # %llu", 
            typeText, &workPlace.count
        );
        if (fields != 2) {
            throw std::runtime_error("Не удалось прочитать запись Work Places");
        }

        trimTrailingSpaces(typeText);
        workPlace.type = parseWorkPlaceType(typeText);

        if (!workPlaces.add(workPlace)) {
            throw std::runtime_error("Таблица рабочих мест Work Places заполнена");
        }
    }
}

void readExchangeRates(ExchangeRateTable& exchangeRates) {
    if (exchangeRates.size() != 0) {
        throw std::invalid_argument("Для чтения Exchange Rates требуется пустая таблица");
    }

    unsigned long long count = 0;
    if (std::scanf(" Exchange Rates %llu", &count) != 1) {
        throw std::runtime_error("Не удалось прочитать заголовок Exchange Rates");
    }
    if (count > ExchangeRateTable::capacity) {
        throw std::runtime_error("Количество курсов превышает вместимость таблицы Exchange Rates");
    }

    for (unsigned long long i = 0; i < count; ++i) {
        ExchangeRate exchangeRate{};
        char soldCurrencyText[100]{};
        char boughtCurrencyText[100]{};
        const int fields = std::scanf(
            " %99[a-zA-Z0-9/_ ] # %99[a-zA-Z0-9/_ ] # %le",
            soldCurrencyText, boughtCurrencyText, &exchangeRate.rate
        );
        if (fields != 3) {
            throw std::runtime_error("Не удалось прочитать запись Exchange Rates");
        }

        trimTrailingSpaces(soldCurrencyText);
        trimTrailingSpaces(boughtCurrencyText);
        exchangeRate.soldCurrency = parseCurrency(soldCurrencyText);
        exchangeRate.boughtCurrency = parseCurrency(boughtCurrencyText);

        if (!exchangeRates.add(exchangeRate)) {
            throw std::runtime_error("Таблица курсов Exchange Rates заполнена");
        }
    }
}

bool readEvent(Event& event) {
    char line[1024]{};
    while (std::scanf(" %1023[^\n]", line) == 1) {
        std::scanf("%*[^\n]");
        if (std::strlen(line) == sizeof(line) - 1) {
            continue;
        }

        Event candidate{};
        int nameOffset = 0;
        // например: 1 # 10:30 # Start of Bank Day
        const int fields = std::sscanf(
            line, // отсюда читаю 
            "%llu # %llu:%llu # %n",
            &candidate.timestamp.date, 
            &candidate.timestamp.time.hour,
            &candidate.timestamp.time.minute, 
            &nameOffset
        );
        if (fields != 3 || nameOffset == 0) {
            continue;
        }

        char* event_name = line + nameOffset;
        trimTrailingSpaces(event_name);
        if (std::strncmp(event_name, "Start of Bank Day", 100) == 0) {
            candidate.type = EventType::StartOfBankDay;
            event = candidate;
            return true;
        }
        if (std::strncmp(event_name, "End of Bank Day", 100) == 0) {
            candidate.type = EventType::EndOfBankDay;
            event = candidate;
            return true;
        }
    }

    if (std::ferror(stdin)) {
        throw std::runtime_error("Ошибка чтения событий");
    }

    return false;
}
