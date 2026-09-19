#include "test_helper.hpp"

TEST_CASE("parse_format действительно распознает форматы JSON, TOML, XML", "[format]") {
    REQUIRE(lab4::parse_format("json") == lab4::Format::Json);
    REQUIRE(lab4::parse_format("TOML") == lab4::Format::Toml);
    REQUIRE(lab4::parse_format("Xml") == lab4::Format::Xml);
    REQUIRE_FALSE(lab4::parse_format("md").has_value());
}

TEST_CASE("to_string обратная операция к parse_format", "[format]") {
    for (auto format : {lab4::Format::Json, lab4::Format::Toml, lab4::Format::Xml}) {
        REQUIRE(lab4::parse_format(lab4::to_string(format)) == format);
    }
}