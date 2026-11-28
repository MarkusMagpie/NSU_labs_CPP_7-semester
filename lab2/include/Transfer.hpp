#pragma once

#include <ComplexTypes.hpp>

// результат попытки перевода денег
enum class TransferResult {
    Success,
    SameAccount,
    DifferentCurrencies,
    InvalidTime,
    InvalidAmount,
    InvalidBalance,
    InsufficientFunds,
    AmountOutOfRange,
    LogError
};

// правило проверки баланса - разрешено ли списание если после него баланс отправителя станет отрицательным?
enum class OverdraftPolicy {
    Disallow,
    Allow
};

/**
 *
 * @param from счет с которого списываем
 * @param to счет на который зачисляем
 * @param amount сумма перевода
 * @param timestamp дата и время для журнала
 * @param overdraft разрешать ли отрицательный баланс отправителя (from)
 */
TransferResult transfer(Account& from, Account& to, Money amount, DateTime timestamp,
    OverdraftPolicy overdraft = OverdraftPolicy::Disallow);
