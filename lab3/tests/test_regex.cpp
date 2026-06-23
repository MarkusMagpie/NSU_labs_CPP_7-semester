#include <catch2/catch_test_macros.hpp>
#include "regex_engine.hpp"

static bool matches(const std::string& pattern, const std::string& text) {
    Regex re(pattern);
    return re.match(text);
}

TEST_CASE("Literal characters match", "[literal]") {
    // REQUIRE(Regex("a").match("a"));
    REQUIRE(matches("a", "a"));
    REQUIRE(matches("abc", "abc"));
    REQUIRE(matches("hello world", "hello world"));
    REQUIRE(matches("hello WORLD!", "hello WORLD!"));
    REQUIRE_FALSE(matches("a", "b"));
    REQUIRE_FALSE(matches("abc", "ab"));
    REQUIRE_FALSE(matches("abc", "abcd"));
    REQUIRE_FALSE(matches("hello WORLD!", "hello world"));
}