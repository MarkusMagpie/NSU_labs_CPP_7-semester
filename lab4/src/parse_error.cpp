#include "parse_error.hpp"

namespace lab4 {
namespace {
std::string format_message(const std::string& message, std::size_t line, std::size_t column) {
    return "ошибка парсинга в line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + message;
}
}  // namespace

ParseError::ParseError(std::string message, std::size_t line, std::size_t column)
    : std::runtime_error(format_message(message, line, column)),
      line_(line),
      column_(column) {}
}  // namespace lab4