#include "AbstractTreeNode.hpp"

namespace lab4 {
// std::variant определит какую из альтернатив активировать в зависимости от переданного в аргументе типа
AbstractTreeNode::AbstractTreeNode(bool value) : data_(value) {}
AbstractTreeNode::AbstractTreeNode(int value) : data_(value) {}
AbstractTreeNode::AbstractTreeNode(double value) : data_(value) {}
AbstractTreeNode::AbstractTreeNode(std::string value) : data_(std::move(value)) {}
AbstractTreeNode::AbstractTreeNode(Array value) : data_(std::move(value)) {}
AbstractTreeNode::AbstractTreeNode(Object value) : data_(std::move(value)) {}


// is_bool/as_bool - обертки над шаблонными is<T>()/as<T>() (см value.hpp)
bool AbstractTreeNode::is_bool() const { return is<bool>(); }
bool AbstractTreeNode::is_int() const { return is<int>(); }
bool AbstractTreeNode::is_double() const { return is<double>(); }
bool AbstractTreeNode::is_string() const { return is<std::string>(); }
bool AbstractTreeNode::is_array() const { return is<Array>(); }
bool AbstractTreeNode::is_object() const { return is<Object>(); }


bool AbstractTreeNode::as_bool() const { return as<bool>(); }
int AbstractTreeNode::as_int() const { return as<int>(); }
double AbstractTreeNode::as_double() const { return as<double>(); }

const std::string& AbstractTreeNode::as_string() const { return as<std::string>(); }
const Array& AbstractTreeNode::as_array() const { return as<Array>(); }
Array& AbstractTreeNode::as_array() { return as<Array>(); }
const Object& AbstractTreeNode::as_object() const { return as<Object>(); }
Object& AbstractTreeNode::as_object() { return as<Object>(); }

}  // namespace lab4