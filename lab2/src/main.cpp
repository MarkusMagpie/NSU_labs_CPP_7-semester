#include <cstdio>
#include <cstdlib>
#include <exception>

#include <AccountTable.hpp>
#include <DepositTable.hpp>
#include <CreditTable.hpp>
#include <ClientTable.hpp>
#include <ClientAccountTable.hpp>
#include <BankAccountTable.hpp>
#include <InputReader.hpp>

int main() {
    try {
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
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Ошибка чтения: %s\n", error.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
