#include <limits>
#include <stdexcept>

#include <DateTime.hpp>

bool isValidTime(const Time& time) {
    return time.hour < 24 && time.minute < 60;
}

int compareDateTime(const DateTime& left, const DateTime& right) {
    if (!isValidTime(left.time) || !isValidTime(right.time)) {
        throw std::invalid_argument("Некорректное время");
    }

    if (left.date != right.date) {
        return left.date < right.date ? -1 : 1;
    }
    if (left.time.hour != right.time.hour) {
        return left.time.hour < right.time.hour ? -1 : 1;
    }
    if (left.time.minute != right.time.minute) {
        return left.time.minute < right.time.minute ? -1 : 1;
    }

    return 0;
}

DateTime addMinutes(const DateTime& value, unsigned long long minutes) {
    if (!isValidTime(value.time)) {
        throw std::invalid_argument("Некорректное время");
    }

    constexpr unsigned long long minutesPerDay = 24 * 60;
    const auto currentMinutes = value.time.hour * 60 + value.time.minute;
    const auto remainingMinutes = currentMinutes + minutes % minutesPerDay;
    const auto addedDays = minutes / minutesPerDay + remainingMinutes / minutesPerDay;

    if (addedDays > std::numeric_limits<Date>::max() - value.date) {
        throw std::overflow_error("Переполнение даты моделирования");
    }

    const auto minuteOfDay = remainingMinutes % minutesPerDay;
    return DateTime{
        value.date + addedDays,
        Time{minuteOfDay / 60, minuteOfDay % 60}
    };
}
