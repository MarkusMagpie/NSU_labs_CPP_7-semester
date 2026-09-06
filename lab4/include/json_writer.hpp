#pragma once

#include <ostream>
#include "value.hpp"

namespace lab4 {
// сериализует дерево Value в JSON
void write_json(const Value& value, std::ostream& out);
}  // namespace lab4