#pragma once

#include <string_view>
#include "AbstractTreeNode.hpp"

namespace lab4 {
// текст в формате XML -> абстрактное дерево AbstractTreeNode
// throws lab4::ParseError если text синтаксически некорректен
AbstractTreeNode parse_xml(std::string text);
} // namespace lab4