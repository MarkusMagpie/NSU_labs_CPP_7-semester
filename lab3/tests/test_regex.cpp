#include <catch2/catch_test_macros.hpp>
#include "regex_engine.hpp"

static bool matches(const std::string& pattern, const std::string& text) {
    Regex re(pattern);
    return re.match(text);
}

TEST_CASE("Literal characters match", "[test]") {
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

TEST_CASE("Dot matches any single character", "[test]") {
    REQUIRE(matches(".", "a"));
    REQUIRE(matches(".", "A"));
    REQUIRE(matches(".", "5"));
    REQUIRE(matches(".", " "));
    REQUIRE(matches(".", "."));
    REQUIRE_FALSE(matches(".", ""));
    REQUIRE_FALSE(matches(".", "abcd"));
}

TEST_CASE("Character group matches any member of this group", "[test]") {
    REQUIRE(matches("[abc]", "a"));
    REQUIRE(matches("[abc]", "b"));
    REQUIRE(matches("[abc]", "c"));
    REQUIRE_FALSE(matches("[abc]", "d"));
    REQUIRE_FALSE(matches("[abc]", ""));
}

TEST_CASE("Character group with range matches any member of this range", "[test]") {
    REQUIRE(matches("[a-z]", "a"));
    REQUIRE(matches("[a-z]", "b"));
    REQUIRE(matches("[a-z]", "z"));
    REQUIRE_FALSE(matches("[a-z]", "A"));
    REQUIRE_FALSE(matches("[a-z]", "67"));
}

TEST_CASE("Character group with multiple ranges", "[test]") {
    REQUIRE(matches("[a-zA-Z0-9]", "a"));
    REQUIRE(matches("[a-zA-Z0-9]", "Z"));
    REQUIRE(matches("[a-zA-Z0-9]", "5"));
}

TEST_CASE("Star matches zero, one or more occurrences", "[test]") {
    REQUIRE(matches("a*", ""));
    REQUIRE(matches("a*", "a"));
    REQUIRE(matches("a*", "aaa"));
    REQUIRE(matches("[abc]*", ""));
    REQUIRE(matches("[abc]*", "abcabc"));
    REQUIRE(matches(".*", ""));
    REQUIRE(matches(".*", "aaaaaaa"));
    REQUIRE(matches(".*", "676767"));
}

TEST_CASE("Star is greedy", "[test]") {
    REQUIRE_FALSE(matches("a*a", "aa"));
    REQUIRE_FALSE(matches("a*a", "a"));
    REQUIRE_FALSE(matches(".*.", "ab"));
}

TEST_CASE("Plus matches one or more occurrences", "[test]") {
    REQUIRE(matches("a+", "a"));
    REQUIRE(matches("a+", "aaa"));
    REQUIRE(matches("[abc]+", "abc"));
    REQUIRE(matches(".+", "a"));
    REQUIRE(matches(".+", "hello"));
    REQUIRE_FALSE(matches("a+", ""));
    REQUIRE_FALSE(matches("[abc]+", ""));
    REQUIRE_FALSE(matches(".+", ""));
}

TEST_CASE("Plus is greedy", "[test]") {
    REQUIRE_FALSE(matches("a+a", "aa"));
    REQUIRE_FALSE(matches("a+a", "a"));
}

TEST_CASE("Question matches zero or one occurrence", "[test]") {
    REQUIRE(matches("a?", ""));
    REQUIRE(matches("a?", "a"));
    REQUIRE(matches("[abc]?", ""));
    REQUIRE(matches("[abc]?", "b"));
    REQUIRE(matches(".?", ""));
    REQUIRE(matches(".?", "x"));
    REQUIRE_FALSE(matches("a?", "aa"));
    REQUIRE_FALSE(matches("[abc]?", "d"));
}

TEST_CASE("Question is lazy", "[test]") {
    REQUIRE(matches("a?a", "a"));
    REQUIRE_FALSE(matches("a?a", ""));
}

TEST_CASE("Combined patterns", "[test]") {
    REQUIRE(matches("[a-z]+[0-9]*", "abc"));
    REQUIRE(matches("[a-z]+[0-9]*", "abc123"));
    REQUIRE_FALSE(matches("[a-z]+[0-9]*", "123"));
    REQUIRE_FALSE(matches("[a-z]+[0-9]*", ""));

    REQUIRE(matches("[A-Z][a-z]+", "Hello"));
    REQUIRE_FALSE(matches("[A-Z][a-z]+", "HEllo"));
    REQUIRE_FALSE(matches("[A-Z][a-z]+", "hello"));
    REQUIRE_FALSE(matches("[A-Z][a-z]+", "H"));

    REQUIRE(matches("[0-9]+.[0-9]+", "3.14"));
    REQUIRE_FALSE(matches("[0-9]+.[0-9]+", "3."));
}