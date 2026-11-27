#pragma once

#include <BankState.hpp>

// возвращают счет из bank.accounts или nullptr если связанный счет не найден
Account* findAccountByCreditId(BankState& bank, unsigned long long creditId);
Account* findAccountByDepositId(BankState& bank, unsigned long long depositId);
Account* findBankAccountByCurrency(BankState& bank, Currency currency);
