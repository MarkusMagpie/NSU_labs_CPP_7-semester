#pragma once

#include <ostream>
#include "AbstractTreeNode.hpp"

namespace lab4 {
// сериализует дерево AbstractTreeNode в JSON
void write_json(const AbstractTreeNode& value, std::ostream& out);
}  // namespace lab4