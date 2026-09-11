#include "xml_writer.hpp"

#include <array>
#include <charconv>
#include <stdexcept>
#include <string>

namespace lab4 {
namespace {
// write_double из toml_writer.cpp
void write_double(double value, std::ostream& out) {
    std::array<char, 64> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    std::string text(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));

    // to_chars выдал double записанный как int -> сам добавляю дробную часть
    if (text.find_first_of(".eE") == std::string::npos) {
        text += ".0";
    }

    out << text;
}

// bool -> "true"/"false", int/double -> число, string -> как есть
void write_scalar_text(const Value& value, std::ostream& out) {
    if (value.is<bool>()) {
        out << (value.as<bool>() ? "true" : "false");
    } else if (value.is<int>()) {
        out << value.as<int>();
    } else if (value.is<double>()) {
        write_double(value.as<double>(), out);
    } else if (value.is<std::string>()) {
        out << value.as<std::string>();
    } else {
        throw std::invalid_argument("write_xml: value не поддерживается (ожидался bool/int/double/string)");
    }
}

// значение атрибута type="..." для конкретного Value - должно совпадать с тем что понимает
// convert_entry_value в xml_parser.cpp
const char* type_name(const Value& value) {
    if (value.is<bool>()) return "bool";
    if (value.is<int>()) return "int";
    if (value.is<double>()) return "double";
    if (value.is<std::string>()) return "string";

    throw std::invalid_argument("write_xml: value не поддерживается (ожидался bool/int/double/string)");
}

// <entry key="..." type="...">значение</entry>
void write_entry(const std::string& key, const Value& value, std::ostream& out) {
    out << "  <entry key=\"" << key << "\" type=\"" << type_name(value) << "\">";
    write_scalar_text(value, out);
    out << "</entry>\n";
}
}  // namespace

void write_xml(const Value& value, std::ostream& out) {
    if (!value.is<Object>()) {
        throw std::invalid_argument("write_xml: value должен быть пользовательского типа данных Object");
    }

    // пролог похуй
    out << "<root>\n";
    for (const auto& [key, member] : value.as<Object>()) {
        write_entry(key, member, out);
    }
    out << "</root>\n";
}
}  // namespace lab4