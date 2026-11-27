#pragma once

#include <AccountTable.hpp>
#include <DepositTable.hpp>
#include <CreditTable.hpp>
#include <ClientTable.hpp>
#include <ClientAccountTable.hpp>
#include <BankAccountTable.hpp>
#include <ClientCreditTable.hpp>
#include <WorkPlaceTable.hpp>
#include <ExchangeRateTable.hpp>
#include <Event.hpp>

// чтение таблиц из stdin в пустые таблицы
void readAccounts(AccountTable& accounts);
void readDeposits(DepositTable& deposits);
void readCredits(CreditTable& credits);
void readClients(ClientTable& clients);
void readClientAccounts(ClientAccountTable& clientAccounts);
void readBankAccounts(BankAccountTable& bankAccounts);
void readClientCredits(ClientCreditTable& clientCredits);
void readWorkPlaces(WorkPlaceTable& workPlaces);
void readExchangeRates(ExchangeRateTable& exchangeRates);

// чтение следующего известного события
bool readEvent(Event& event);
