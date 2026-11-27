#include <BankLookup.hpp>

// счет по id кредита
/*
    при закрытии дня надо искать кредиты типа Charged Daily. 
        в структуре Credit нету номера связанного счета - его и ищу
        счет нужен чтобы узнать его баланс и валюту, а затем начислить проценты со счета кредита в пользу банка
*/
Account* findAccountByCreditId(BankState& bank, unsigned long long creditId) {
    for (std::size_t i = 0; i < bank.clientCredits.size(); ++i) {
        const ClientCredit* clientCredit = bank.clientCredits.get(i);
        if (clientCredit->creditId == creditId) {
            return bank.accounts.findById(clientCredit->accountId);
        }
    }

    return nullptr;
}

Account* findAccountByDepositId(BankState& bank, unsigned long long depositId) {
    for (std::size_t i = 0; i < bank.clientAccounts.size(); ++i) {
        const ClientAccount* clientAccount = bank.clientAccounts.get(i);
        if (clientAccount->depositId == depositId) {
            return bank.accounts.findById(clientAccount->accountId);
        }
    }

    return nullptr;
}

Account* findBankAccountByCurrency(BankState& bank, Currency currency) {
    for (std::size_t i = 0; i < bank.bankAccounts.size(); ++i) {
        const BankAccount* bankAccount = bank.bankAccounts.get(i);
        // поиск счета банка среди всех счетов
        Account* account = bank.accounts.findById(bankAccount->accountId);
        if (account != nullptr && account->currency == currency) {
            return account;
        }
    }

    return nullptr;
}
