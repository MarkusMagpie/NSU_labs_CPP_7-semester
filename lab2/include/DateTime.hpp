#pragma once

#include <BasicTypes.hpp>

// часы должны быть от 0 до 23, минуты от 0 до 59
bool isValidTime(const Time& time);

// -1: left раньше right, 0: равны, 1: left позже right
// !isValidTime -> std::invalid_argument
int compareDateTime(const DateTime& left, const DateTime& right);

// возвращает новый день и время после прибавления указанного числа минут
/*
пример использования: 
    день 5, 23:50 + 20 минут = день 6, 00:10
*/
DateTime addMinutes(const DateTime& value, unsigned long long minutes);
