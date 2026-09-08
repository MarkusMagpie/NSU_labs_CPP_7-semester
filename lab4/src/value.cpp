#include "value.hpp"

namespace lab4 {

// std::variant определит какую из альтернатив активировать в зависимости от переданного в аргументе типа
Value::Value(bool value) : data_(value) {}
Value::Value(int value) : data_(value) {}
Value::Value(double value) : data_(value) {}
Value::Value(std::string value) : data_(std::move(value)) {}
Value::Value(Array value) : data_(std::move(value)) {}
Value::Value(Object value) : data_(std::move(value)) {}


// is_bool/as_bool - обертки над шаблонными is<T>()/as<T>() (см value.hpp)
bool Value::is_bool() const { return is<bool>(); }
bool Value::is_int() const { return is<int>(); }
bool Value::is_double() const { return is<double>(); }
bool Value::is_string() const { return is<std::string>(); }
bool Value::is_array() const { return is<Array>(); }
bool Value::is_object() const { return is<Object>(); }


bool Value::as_bool() const { return as<bool>(); }
int Value::as_int() const { return as<int>(); }
double Value::as_double() const { return as<double>(); }

const std::string& Value::as_string() const { return as<std::string>(); }
const Array& Value::as_array() const { return as<Array>(); }
Array& Value::as_array() { return as<Array>(); }
const Object& Value::as_object() const { return as<Object>(); }
Object& Value::as_object() { return as<Object>(); }

}  // namespace lab4