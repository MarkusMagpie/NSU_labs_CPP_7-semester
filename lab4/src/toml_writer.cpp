#include "toml_writer.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <stdexcept>
#include <string>

namespace lab4 {
namespace {
// проверка алфавита из TomlParser::parse_key()
bool is_bare_key(const std::string& key) {
    if (key.empty()) return false;

    for (unsigned char c : key) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }

    return true;
}

// обратная операция к TomlParser::parse_escape_seq - экранирует те же спецсимволы что и JSON writer
void write_escaped_string(const std::string& text, std::ostream& out) {
    out << '"';
    for (unsigned char c : text) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                out << static_cast<char>(c);
        }
    }
    out << '"';
}

void write_key(const std::string& key, std::ostream& out) {
    if (is_bare_key(key)) { // [A-Za-z0-9_-]+
        out << key;
    } else {
        write_escaped_string(key, out); // "my key" -> пишем в кавычках, как quoted key
    }
}

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

// запись value
void write_scalar(const Value& value, std::ostream& out) {
    if (value.is<bool>()) {
        out << (value.as<bool>() ? "true" : "false");
    } else if (value.is<int>()) {
        out << value.as<int>();
    } else if (value.is<double>()) {
        write_double(value.as<double>(), out);
    } else if (value.is<std::string>()) {
        write_escaped_string(value.as<std::string>(), out);
    } else {
        throw std::invalid_argument(
            "write_toml: gjrf yt gjllth;bdftnccz");
    }
}
}  // namespace

void write_toml(const Value& value, std::ostream& out) {
    if (!value.is<Object>()) {
        throw std::invalid_argument("write_toml: value должен быть таблицей (тип данных Object)");
    }

    for (const auto& [key, member] : value.as<Object>()) {
        write_key(key, out);
        out << " = ";
        write_scalar(member, out);
        out << "\n";
    }
}
}  // namespace lab4