#include "value.hpp"

namespace lab4 {

// std::variant определит какую из альтернатив активировать в зависимости от переданного в аргументе типа
Value::Value(bool value) : data_(value) {}
Value::Value(int value) : data_(value) {}
Value::Value(double value) : data_(value) {}
Value::Value(std::string value) : data_(std::move(value)) {}
Value::Value(Array value) : data_(std::move(value)) {}
Value::Value(Object value) : data_(std::move(value)) {}


bool Value::is_bool() const {
    return std::holds_alternative<bool>(data_);
}
bool Value::is_int() const {
    return std::holds_alternative<int>(data_);
}
bool Value::is_double() const {
    return std::holds_alternative<double>(data_);
}
bool Value::is_string() const {
    return std::holds_alternative<std::string>(data_);
}
bool Value::is_array() const {
    return std::holds_alternative<Array>(data_);
}
bool Value::is_object() const {
    return std::holds_alternative<Object>(data_);
}


bool Value::as_bool() const { return std::get<bool>(data_); }
int Value::as_int() const { return std::get<int>(data_); }
double Value::as_double() const { return std::get<double>(data_); }

const std::string& Value::as_string() const { return std::get<std::string>(data_); }
const Array& Value::as_array() const { return std::get<Array>(data_); }
Array& Value::as_array() { return std::get<Array>(data_); }
const Object& Value::as_object() const { return std::get<Object>(data_); }
Object& Value::as_object() { return std::get<Object>(data_); }

}  // namespace lab4