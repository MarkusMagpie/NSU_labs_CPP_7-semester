#pragma once

#include <ostream>
#include "AbstractTreeNode.hpp"

namespace lab4 {
// сериализует дерево AbstractTreeNode в XML объект
void write_xml(const AbstractTreeNode& value, std::ostream& out);
}  // namespace lab4