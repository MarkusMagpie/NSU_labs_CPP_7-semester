#include <cstdio>
#include <cstdlib>
#include <exception>

#include <BankState.hpp>
#include <EventProcessor.hpp>
#include <InputReader.hpp>

int main() {
    try {
        /* --- Чтение всех начальных таблиц из stdin --- */
        BankState bank;
        readAccounts(bank.accounts);

        readDeposits(bank.deposits);

        readCredits(bank.credits);

        readClients(bank.clients);

        readClientAccounts(bank.clientAccounts);

        readBankAccounts(bank.bankAccounts);

        readClientCredits(bank.clientCredits);

        readWorkPlaces(bank.workPlaces);

        readExchangeRates(bank.exchangeRates);
        /* --------------------------------------------- */

        Event event{};
        while (readEvent(event)) {
            processEvent(bank, event);
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Ошибка чтения: %s\n", error.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
