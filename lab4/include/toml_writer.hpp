#pragma once

#include <ostream>
#include "value.hpp"

namespace lab4 {
// сериализует дерево Value в TOML
// value должен быть Object (TOML документ - это таблица "key = value"), а значения внутри -
// bool/int/double/string.
// Массивы и вложенные таблицы/объекты не поддерживаются
void write_toml(const Value& value, std::ostream& out);
}  // namespace lab4