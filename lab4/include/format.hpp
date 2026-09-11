#pragma once

#include <optional>
#include <string_view>
#include <string> // для to_lower

namespace lab4 {
enum class Format {Json, Toml,Xml};
std::optional<Format> parse_format(std::string name);
std::string to_string(Format format); // обратная операция к parse_format
} // namespace lab4
