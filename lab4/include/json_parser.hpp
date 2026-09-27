#pragma once

#include <string_view>
#include <cctype>
#include <stdexcept>
#include "AbstractTreeNode.hpp"

namespace lab4 {
// текст в формате JSON -> абстрактное дерево AbstractTreeNode
// throws lab4::ParseError если text синтаксически некорректен
AbstractTreeNode parse_json(std::string text);
}  // namespace lab4