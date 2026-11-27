#pragma once

#include <BasicTypes.hpp>

enum class EventType {
    StartOfBankDay,
    EndOfBankDay
};

struct Event {
    DateTime timestamp;
    EventType type;
};
