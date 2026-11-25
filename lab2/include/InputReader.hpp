#pragma once

#include <AccountTable.hpp>
#include <DepositTable.hpp>
#include <CreditTable.hpp>
#include <ClientTable.hpp>

// чтение таблиц из stdin в пустые таблицы
void readAccounts(AccountTable& accounts);
void readDeposits(DepositTable& deposits);
void readCredits(CreditTable& credits);
void readClients(ClientTable& clients);
