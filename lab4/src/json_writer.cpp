#include "json_writer.hpp"

#include <array>
#include <charconv>
#include <cstdio>
#include <string>

namespace lab4 {
namespace {
// обратная операция к parse_string_raw/parse_escape из json_parser.cpp
void write_escaped_string(const std::string& text, std::ostream& out) {
    out << '"';
    for (unsigned char c : text) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            // "\n" в C++ - escape-sequence, дающая один байт. В out (json) нужно написать ее текстовое представление -> экранирую \ в \\, n без изменений
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                out << static_cast<char>(c);
        }
    }
    out << '"';
}

void write_double(double value, std::ostream& out) {
    std::array<char, 64> buffer{}; // 64 байта. инициализированы нулями
    // конвертация числа value в текст в buffer.data()
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    std::string text(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));

    // to_chars выдал double записанный как int -> сам добавляю дробную часть
    if (text.find_first_of(".eE") == std::string::npos) {
        text += ".0";
    }

    out << text;
}
}  // namespace

void write_json(const Value& value, std::ostream& out) {
    if (value.is_bool()) {
        out << (value.as_bool() ? "true" : "false");
    } else if (value.is_int()) {
        out << value.as_int();
    } else if (value.is_double()) {
        write_double(value.as_double(), out);
    } else if (value.is_string()) {
        write_escaped_string(value.as_string(), out);
    } else if (value.is_array()) {
        out << '[';
        const Array& items = value.as_array();
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i != 0) out << ',';
            write_json(items[i], out);
        }
        out << ']';
    } else if (value.is_object()) {
        out << '{';
        const Object& members = value.as_object();
        for (std::size_t i = 0; i < members.size(); ++i) {
            if (i != 0) out << ',';
            write_escaped_string(members[i].first, out);
            out << ':';
            write_json(members[i].second, out);
        }
        out << '}';
    }
}
}  // namespace lab4