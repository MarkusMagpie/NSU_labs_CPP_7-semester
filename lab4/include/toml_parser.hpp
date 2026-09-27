#pragma once

#include <string_view>
#include "AbstractTreeNode.hpp"

namespace lab4 {
// текст в формате TOML -> абстрактное дерево AbstractTreeNode
// поддерживается плоский набор строк "key = value"
// value/значение: bool/int/double/string.
// массивы, inline-таблицы, [table]-заголовки, datetime не поддерживаются

// throws lab4::ParseError если text синтаксически некорректен
AbstractTreeNode parse_toml(std::string_view text);
}  // namespace lab4