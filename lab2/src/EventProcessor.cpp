#include <stdexcept>

#include <DateTime.hpp>
#include <EventProcessor.hpp>

void processEvent(BankState& bank, const Event& event) {
    if (!isValidTime(event.timestamp.time)) {
        throw std::invalid_argument("Указано некорректное время события");
    }

    switch (event.type) {
        case EventType::StartOfBankDay:
            // это фактическое начало дня
            bank.bankDayStart = DateTime{
                event.timestamp.date, 
                Time{8, 0}
            };
            bank.bankDayStarted = true;
            bank.bankDayEnded = false;

            return;

        case EventType::EndOfBankDay:
            // Независимо от времени прихода данного события банковский день заканчивается в 19:00. 
            bank.bankDayEnd = DateTime{
                event.timestamp.date,
                Time{19, 0}
            };
            bank.bankDayEnded = true;

            // начисления процентов, статистика и завершение обслуживания пока не реализованы
            return;
    }

    throw std::invalid_argument("Неизвестный тип события");
}
