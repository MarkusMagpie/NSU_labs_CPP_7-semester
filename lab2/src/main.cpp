#include <cstdio>
#include <cstdlib>
#include <exception>

#include <AccountTable.hpp>
#include <DepositTable.hpp>
#include <CreditTable.hpp>
#include <ClientTable.hpp>
#include <ClientAccountTable.hpp>
#include <BankAccountTable.hpp>
#include <ClientCreditTable.hpp>
#include <WorkPlaceTable.hpp>
#include <ExchangeRateTable.hpp>
#include <InputReader.hpp>

int main() {
    try {
        /* --- Чтение всех начальных таблиц из stdin --- */
        AccountTable accounts;
        readAccounts(accounts);

        DepositTable deposits;
        readDeposits(deposits);

        CreditTable credits;
        readCredits(credits);

        ClientTable clients;
        readClients(clients);

        ClientAccountTable clientAccounts;
        readClientAccounts(clientAccounts);

        BankAccountTable bankAccounts;
        readBankAccounts(bankAccounts);

        ClientCreditTable clientCredits;
        readClientCredits(clientCredits);

        WorkPlaceTable workPlaces;
        readWorkPlaces(workPlaces);

        ExchangeRateTable exchangeRates;
        readExchangeRates(exchangeRates);
        /* --------------------------------------------- */
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Ошибка чтения: %s\n", error.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
