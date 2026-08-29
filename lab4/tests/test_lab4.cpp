#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>
#include "format.hpp"
#include "io_utils.hpp"

TEST_CASE("parse_format recognizes known formats", "[format]") {
    REQUIRE(lab4::parse_format("json") == lab4::Format::Json);
    REQUIRE(lab4::parse_format("TOML") == lab4::Format::Toml);
    REQUIRE(lab4::parse_format("Xml") == lab4::Format::Xml);
    REQUIRE_FALSE(lab4::parse_format("md").has_value());
}

TEST_CASE("to_string is the inverse of parse_format", "[format]") {
    for (auto format : {lab4::Format::Json, lab4::Format::Toml, lab4::Format::Xml}) {
        REQUIRE(lab4::parse_format(lab4::to_string(format)) == format);
    }
}

TEST_CASE("read_all reads the whole stream including newlines", "[io]") {
    std::istringstream input("line1\nline2\n");
    REQUIRE(lab4::read_all(input) == "line1\nline2\n");
}