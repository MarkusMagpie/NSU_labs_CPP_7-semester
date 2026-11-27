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

// таблицы составных данных и изменяемое состояние банковской модели
struct BankState {
    AccountTable accounts;
    DepositTable deposits;
    CreditTable credits;
    ClientTable clients;
    ClientAccountTable clientAccounts;
    BankAccountTable bankAccounts;
    ClientCreditTable clientCredits;
    WorkPlaceTable workPlaces;
    ExchangeRateTable exchangeRates;

    bool bankDayStarted = false; // получено событие начала банковского дня?
    DateTime bankDayStart{}; // фактическое начало дня: дата события, 08:00
    bool bankDayEnded = false; // получено событие конца текущего банковского дня?
    DateTime bankDayEnd{}; // фактический конец дня: дата события, 19:00
};
