#pragma once

#include <string_view>
#include <cctype>
#include <stdexcept>
#include "value.hpp"

namespace lab4 {
// текст в формате JSON -> абстрактное дерево Value
// throws lab4::ParseError если text синтаксически некорректен
Value parse_json(std::string text);

}  // namespace lab4