#pragma once

#include <AccountTable.hpp>
#include <DepositTable.hpp>

// чтение таблицы Accounts из stdin в пустую таблицу счетов
void readAccounts(AccountTable& accounts);

// чтение таблицы Deposits из stdin в пустую таблицу вкладов
void readDeposits(DepositTable& deposits);
