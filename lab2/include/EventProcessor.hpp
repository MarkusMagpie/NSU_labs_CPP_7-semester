#pragma once

#include <BankState.hpp>
#include <Event.hpp>

// применение события к состоянию банка
void processEvent(BankState& bank, const Event& event);
