#include <cstdio>
#include <limits>
#include <stdexcept>

#include <DateTime.hpp>
#include <Money.hpp>
#include <Transfer.hpp>

namespace {
// сравнение модулей сумм: |left| < |right| -> -1; |left| > |right| -> 1
int compareMagnitude(const MoneyParts& left, const MoneyParts& right) {
    if (left.whole != right.whole) {
        return left.whole < right.whole ? -1 : 1;
    }
    if (left.thousandths != right.thousandths) {
        return left.thousandths < right.thousandths ? -1 : 1;
    }
    
    return 0;
}

// складывает денежные суммы по частям
MoneyParts addParts(const MoneyParts& left, const MoneyParts& right) {
    MoneyParts result{};
    if (left.negative == right.negative) {
        // знаки одинаковые -> складываю модули
        result.thousandths = left.thousandths + right.thousandths;
        const unsigned long long carry = result.thousandths / 1000;
        result.thousandths %= 1000;
        
        const auto max = std::numeric_limits<unsigned long long>::max();
        if (left.whole > max - right.whole || left.whole + right.whole > max - carry) {
            throw std::overflow_error("Переполнение денежной суммы");
        }

        result.whole = left.whole + right.whole + carry;
        result.negative = left.negative;
    } else {
        // знаки разные -> вычитаю меньший модуль из большего
        const int comparison = compareMagnitude(left, right); // у кого больше модуль?
        const MoneyParts& larger = comparison >= 0 ? left : right;
        const MoneyParts& smaller = comparison >= 0 ? right : left;

        result.whole = larger.whole - smaller.whole;
        
        if (larger.thousandths < smaller.thousandths) {
            --result.whole;
            result.thousandths = larger.thousandths + 1000 - smaller.thousandths;
        } else {
            result.thousandths = larger.thousandths - smaller.thousandths;
        }

        // знак беру у числа с большим модулем
        result.negative = larger.negative && (result.whole != 0 || result.thousandths != 0);
    }

    return result;
}

Money fromParts(const MoneyParts& parts) {
    return moneyFromParts(parts.whole, parts.thousandths, parts.negative);
}
} // namespace



TransferResult transfer(Account& from, Account& to, Money amount, DateTime timestamp, 
    OverdraftPolicy overdraft) 
{
    if (&from == &to || from.id == to.id) return TransferResult::SameAccount;
    if (from.currency != to.currency) return TransferResult::DifferentCurrencies;
    if (!isValidTime(timestamp.time)) return TransferResult::InvalidTime;

    // сумма перевода amount в MoneyParts
    MoneyParts amountParts{};
    try {
        amountParts = moneyToParts(amount);
    } catch (const std::invalid_argument&) {
        return TransferResult::InvalidAmount;
    } catch (const std::overflow_error&) {
        return TransferResult::InvalidAmount;
    }
    // сумма денег которую запрашивает счет у другого строго положительна
    if (amountParts.negative || (amountParts.whole == 0 && amountParts.thousandths == 0)) {
        return TransferResult::InvalidAmount;
    }

    /* проверка обоих балансов (объемов средств на счете) */
    MoneyParts fromPartsValue{};
    MoneyParts toPartsValue{};
    try {
        fromPartsValue = moneyToParts(from.balance);
        toPartsValue = moneyToParts(to.balance);
    } catch (const std::invalid_argument&) {
        return TransferResult::InvalidBalance;
    } catch (const std::overflow_error&) {
        return TransferResult::InvalidBalance;
    }
    /* -------------------------------------------------- */

    // по умолчанию у отправителя должно хватать денег -> если OverdraftPolicy::Disallow && если баланс меньше amount -> InsufficientFunds
    if (overdraft == OverdraftPolicy::Disallow &&
        (fromPartsValue.negative || compareMagnitude(fromPartsValue, amountParts) < 0)) {
        return TransferResult::InsufficientFunds;
    }

    // вычисление новых/обновленных балансов
    Money newFromBalance;
    Money newToBalance;
    try {
        // баланс отправителя
        MoneyParts negativeAmount = amountParts;
        negativeAmount.negative = true;
        newFromBalance = fromParts(addParts(fromPartsValue, negativeAmount));

        // баланс получателя
        newToBalance = fromParts(addParts(toPartsValue, amountParts));
    } catch (const std::invalid_argument&) {
        return TransferResult::AmountOutOfRange;
    } catch (const std::overflow_error&) {
        return TransferResult::AmountOutOfRange;
    }

    // запись в stderr как требует ТЗ для журнала транзакций
    // например: 5 # 19:0 # 101 -> 201 # 2.350
    if (std::fprintf(stderr, "%llu # %llu:%llu # %llu -> %llu # %llu.%03llu\n",
                     timestamp.date, timestamp.time.hour, timestamp.time.minute,
                     from.id, to.id, amountParts.whole, amountParts.thousandths) < 0) {
        return TransferResult::LogError;
    }

    from.balance = newFromBalance;
    to.balance = newToBalance;

    return TransferResult::Success;
}
