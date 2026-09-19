#include "test_helper.hpp"

// ТЕСТЫ TOML ПАРСЕРА ---------------------------------------------------------------------------------------------------
TEST_CASE("parse_toml читает bool литералы", "[toml]") {
    lab4::Value value1 = lab4::parse_toml("a = true").as_object()[0].second;
    REQUIRE(value1.as_bool() == true);
    REQUIRE(lab4::parse_toml("a = false").as_object()[0].second.as_bool() == false);
}

TEST_CASE("parse_toml читает int (включая ведущий '+', которого нет в JSON)", "[toml]") {
    lab4::Value v = lab4::parse_toml("a = 0\nb = 100\nc = -100\nd = +5");
    lab4::Object& obj = v.as_object();

    REQUIRE(obj[0].second.as_int() == 0);
    REQUIRE(obj[1].second.as_int() == 100);
    REQUIRE(obj[2].second.as_int() == -100);
    REQUIRE(obj[3].second.as_int() == 5);
}

TEST_CASE("parse_toml читает double", "[toml]") {
    lab4::Value v = lab4::parse_toml("a = 3.14\nb = -1.0e3");
    lab4::Object& obj = v.as_object();

    REQUIRE(obj[0].second.as_double() == 3.14);
    REQUIRE(obj[1].second.as_double() == -1000.0);
}

TEST_CASE("parse_toml читает string и экранированные спецсимволы", "[toml]") {
    lab4::Value v = lab4::parse_toml("a = \"hello\"\nb = \"hello\\nworld\"");
    lab4::Object& obj = v.as_object();

    REQUIRE(obj[0].second.as_string() == "hello");
    REQUIRE(obj[1].second.as_string() == "hello\nworld");
}

TEST_CASE("parse_toml игнорит пустые строки и комментарии + сохраняет порядок ключей", "[toml]") {
    lab4::Value v = lab4::parse_toml("# comment\n\n   a = 1   # comment comment2\n\nb = 2");
    lab4::Object& obj = v.as_object();

    REQUIRE(obj.size() == 2);
    REQUIRE(obj[0].first == "a");
    REQUIRE(obj[0].second.as_int() == 1);
    REQUIRE(obj[1].first == "b");
    REQUIRE(obj[1].second.as_int() == 2);
}

TEST_CASE("parse_toml поддерживает quoted key", "[toml]") {
    lab4::Value v = lab4::parse_toml("\"my key\" = 1\nb = 2");
    lab4::Object& obj = v.as_object();

    REQUIRE(obj[0].first == "my key");
    REQUIRE(obj[1].first == "b");
}

TEST_CASE("parse_toml обрабатывает ошибки", "[toml]") {
    REQUIRE_THROWS_AS(lab4::parse_toml("a = "), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_toml("a 1"), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_toml("= 1"), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_toml("a = 1 2"), lab4::ParseError);
}



// ТЕСТЫ TOML WRITER (интересуют только переходы JSON -> TOML) ---------------------------------------------------------
TEST_CASE("write_toml: json -> toml для скалярных значений (bool/int/double/string)", "[toml][writer]") {
    REQUIRE(json_to_toml(R"({"a": true, "b": 1, "c": 3.14, "d": "hello"})")
        == "a = true\nb = 1\nc = 3.14\nd = \"hello\"\n");
}

TEST_CASE("write_toml: json -> toml пишет ключ с пробелом в кавычках", "[toml][writer]") {
    REQUIRE(json_to_toml(R"({"my key": 1})") == "\"my key\" = 1\n");
}
// parse_json(...) -> Value = Object{ ("my key", Value(1)) }
// write_toml(...) -> write_key("my key", out) -> write_escaped_string("my key", out)
// ",m,y, ,k,e,y,", ,=, ,1

TEST_CASE("write_toml: json -> toml: TOML writer экранирует спецсимволы строки так же как JSON writer", "[toml][writer]") {
    REQUIRE(json_to_toml(R"({"a": "a\"b\\c\nd"})") == "a = \"a\\\"b\\\\c\\nd\"\n");
}
// parse_escape_seq в json_parser.cpp: \" -> ", \\ -> \, \n -> байт перехода строки.
// Value со строкой из 7 настоящих байт: a, ", b, \, c, <newline>, d
// результат: a,  , =,  , ", a, \, ", b, \, \, c, \, n, d, ", <newline-байт>



// TOML ROUNDTRIP в json и обратно -------------------------------------------------------------------------------------
TEST_CASE("json -> toml -> json: скалярные значения переживают round-trip", "[toml][roundtrip]") {
    std::string canon = R"({"a":true,"b":1,"c":3.14,"d":"hello"})";
    REQUIRE(json_via_toml(canon) == canon);
}

TEST_CASE("json -> toml -> json: ключ с пробелом и экранированная строка переживают round-trip", "[toml][roundtrip]") {
    std::string canon = R"({"my key":-100,"text":"a\"b\\c\nd"})";
    // parse_escape_seq в json_parser.cpp превратит второй value в: a " b \  c <newline> d = 7 байт
    // в write_toml эти сиволы буудт сновва экранированы; потом parse_toml, write_json заново экранирует 7 байт и на выходе будет исходный "a\"b\\c\nd"
    REQUIRE(json_via_toml(canon) == canon);
}