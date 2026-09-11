#pragma once

#include <ostream>
#include "value.hpp"

namespace lab4 {
// сериализует дерево Value в XML объект
void write_xml(const Value& value, std::ostream& out);
}  // namespace lab4