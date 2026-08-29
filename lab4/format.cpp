#include "format.hpp"
#include <algorithm>
#include <cctype>

namespace lab4 {

namespace {

std::string to_lower(std::string text) {
    // "пройди по диапазону, к каждому элементу примени функцию, результат положи в другой (или тот же) диапазон"
    std::transform(text.begin(), text.end(),
        text.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

    return text;
}

}  // namespace

std::optional<Format> parse_format(std::string name) {
    const std::string lowered = to_lower(name);
    if (lowered == "json") return Format::Json;
    if (lowered == "toml") return Format::Toml;
    if (lowered == "xml") return Format::Xml;

    return std::nullopt;
}

std::string to_string(Format format) {
    switch (format) {
        case Format::Json: return "json";
        case Format::Toml: return "toml";
        case Format::Xml: return "xml";
    }

    return "unknown";
}

}  // namespace lab4
