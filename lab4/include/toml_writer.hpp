#pragma once

#include <ostream>
#include "AbstractTreeNode.hpp"

namespace lab4 {
// сериализует дерево AbstractTreeNode в TOML
// value должен быть Object (TOML документ - это таблица "key = value"), а значения внутри -
// bool/int/double/string.
// Массивы и вложенные таблицы/объекты не поддерживаются
void write_toml(const AbstractTreeNode& value, std::ostream& out);
}  // namespace lab4