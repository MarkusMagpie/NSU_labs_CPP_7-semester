#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace lab4 {
// ошибка синтаксического разбора документа JSON/TOML/XML
class ParseError : public std::runtime_error {
private:
    std::size_t line_;
    std::size_t column_;
public:
    ParseError(std::string message, std::size_t line, std::size_t column);

    std::size_t line() const { return line_; }
    std::size_t column() const { return column_; }
};
}  // namespace lab4