#pragma once

#include <string_view>
#include "value.hpp"

namespace lab4 {
// текст в формате XML -> абстрактное дерево Value
// throws lab4::ParseError если text синтаксически некорректен
Value parse_xml(std::string text);
} // namespace lab4