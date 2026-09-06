#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace lab4 {
class Value;
// Array=массив - упорядоченный список значений ([1,2,3]; ["a","b","c"], [1,"a",true])
using Array = std::vector<Value>;
// Object = Объект - упорядоченный список пар "ключ, значение": {"name": "vasya", "age": 20}
using Object = std::vector<std::pair<std::string, Value>>;
// Value = узел абстрактного дерева
class Value {
public:
    using Storage = std::variant<bool, int, double, std::string, Array, Object>;

    Value(bool value);
    Value(int value);
    Value(double value);
    Value(std::string value);
    Value(Array value);
    Value(Object value);

    bool is_bool() const;
    bool is_int() const;
    bool is_double() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    bool as_bool() const;
    int as_int() const;
    double as_double() const;
    const std::string& as_string() const;
    const Array& as_array() const;
    Array& as_array();
    const Object& as_object() const;
    Object& as_object();
private:
    Storage data_;
};
}  // namespace lab4