#pragma once

#include <optional>
#include <string_view>
#include <string> // для to_lower

namespace lab4 {

// форматы представления документа в рамках задачи
enum class Format { Json, Toml, Xml };
std::optional<Format> parse_format(std::string name);
// обратная операция к parse_format
std::string to_string(Format format);

}  // namespace lab4
