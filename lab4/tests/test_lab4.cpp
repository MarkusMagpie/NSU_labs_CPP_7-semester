#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include "format.hpp"
#include "io_utils.hpp"
#include "json_parser.hpp"
#include "json_writer.hpp"
#include "parse_error.hpp"
#include "toml_parser.hpp"
#include "toml_writer.hpp"
#include "value.hpp"

TEST_CASE("parse_format recognizes known formats", "[format]") {
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



// ТЕСТЫ JSON ПАРСЕРА --------------------------------------------------------------------------------------------------
TEST_CASE("parse_json читает bool литералы", "[json]") {
    REQUIRE(lab4::parse_json("true").as<bool>() == true); // строка "true" превращается в Value у которого as_bool() возвращает true
    REQUIRE(lab4::parse_json("false").as_bool() == false);
}

TEST_CASE("parse_json читает int", "[json]") {
    REQUIRE(lab4::parse_json("0").as<int>() == 0);
    REQUIRE(lab4::parse_json("100").as_int() == 100);
    REQUIRE(lab4::parse_json("-100").as_int() == -100);
}

TEST_CASE("parse_json reads doubles", "[json]") {
    REQUIRE(lab4::parse_json("3.14").as<double>() == 3.14);
    REQUIRE(lab4::parse_json("-1.0e3").as_double() == -1000.0);
}

TEST_CASE("parse_json игнорит пробелы", "[json]") {
    // чек skip_whitespace() - что пробелы до и после значения не мешают парсингу и не идут в результат как часть значения
    REQUIRE(lab4::parse_json("  42     ").as<int>() == 42);
}

TEST_CASE("parse_json reports errors", "[json]") {
    REQUIRE_THROWS_AS(lab4::parse_json(""), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_json("42 43"), lab4::ParseError);
}

TEST_CASE("parse_json читает одномерный массив/array (пока без вложенности)", "[json]") {
    const lab4::Value v = lab4::parse_json("[1, 2, 3]");

    REQUIRE(v.is_array());
    REQUIRE(v.as_array().size() == 3);
    REQUIRE(v.as_array()[0].as<int>() == 1);
    REQUIRE(v.as_array()[1].as_int() == 2);
    REQUIRE(v.as_array()[2].as_int() == 3);
}

TEST_CASE("parse_json читает object без вложенностей и сохраняет установленный порядок ключей", "[json]") {
    // Raw string literal для удобства записи json
    lab4::Value v = lab4::parse_json(R"({"b": 1, "a": 2})"); // альтернатива: "{\"b\": 1, \"a\": 2}
    REQUIRE(v.is_object());

    lab4::Object& obj = v.as<lab4::Object>();
    REQUIRE(obj.size() == 2);
    REQUIRE(obj[0].first == "b");
    REQUIRE(obj[0].second.as_int() == 1);
    REQUIRE(obj[1].first == "a");
    REQUIRE(obj[1].second.as_int() == 2);
}

TEST_CASE("parse_json правильно читает вложенные arrays + objects", "[json]") {
    const lab4::Value v = lab4::parse_json(R"({"name": "Vasya", "grades": [4, 5, 5, 3], "address": {"city": "Novosibirsk"}})");
    REQUIRE(v.as_object()[0].second.as<std::string>() == "Vasya");

    const lab4::Array& grades = v.as_object()[1].second.as_array();
    REQUIRE(grades.size() == 4);
    REQUIRE(grades[0].as<int>() == 4);
    REQUIRE(grades[1].as_int() == 5);
    REQUIRE(grades[2].as_int() == 5);
    REQUIRE(grades[3].as_int() == 3);

    const lab4::Object& address = v.as_object()[2].second.as_object();
    REQUIRE(address[0].first == "city");
    REQUIRE(address[0].second.as_string() == "Novosibirsk");
}

TEST_CASE("parse_json rejects malformed arrays and objects", "[json]") {
    REQUIRE_THROWS_AS(lab4::parse_json("[1,]"), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_json(R"({"a":1,})"), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_json("{a:1}"), lab4::ParseError); // ключ не строка
    REQUIRE_THROWS_AS(lab4::parse_json(R"({"a" 1})"), lab4::ParseError); // нет ':'
    REQUIRE_THROWS_AS(lab4::parse_json("[1 2]"), lab4::ParseError); // у массива нет ','
    REQUIRE_THROWS_AS(lab4::parse_json("[1,2"), lab4::ParseError); // не закрыт массив
    REQUIRE_THROWS_AS(lab4::parse_json(R"({"a":1)"), lab4::ParseError);  // не закрыт объект
}

// ТЕСТЫ JSON WRITER ---------------------------------------------------------------------------------------------------
namespace {
    std::string write_json_to_string(const lab4::Value& value) {
        std::ostringstream out;
        lab4::write_json(value, out);
        return out.str();
    }
}// namespace

TEST_CASE("write_json пишет bool литералы", "[json][writer]") {
    REQUIRE(write_json_to_string(lab4::Value(true)) == "true");
    REQUIRE(write_json_to_string(lab4::Value(false)) == "false");
}

TEST_CASE("write_json пишет int", "[json][writer]") {
    REQUIRE(write_json_to_string(lab4::Value(0)) == "0");
    REQUIRE(write_json_to_string(lab4::Value(100)) == "100");
    REQUIRE(write_json_to_string(lab4::Value(-100)) == "-100");
}

TEST_CASE("write_json пишет double и всегда добавляет дробную часть", "[json][writer]") {
    REQUIRE(write_json_to_string(lab4::Value(3.14)) == "3.14");
    REQUIRE(write_json_to_string(lab4::Value(-1000.0)) == "-1000.0"); // проверка ветки text += ".0"
}

TEST_CASE("write_json экранирует спецсимволы строки", "[json][writer]") {
    REQUIRE(write_json_to_string(lab4::Value(std::string("hello"))) == "\"hello\"");
    REQUIRE(write_json_to_string(lab4::Value(std::string("a\"b"))) == R"("a\"b")"); // a"b
    REQUIRE(write_json_to_string(lab4::Value(std::string("a\\b"))) == R"("a\\b")"); // a\b
    REQUIRE(write_json_to_string(lab4::Value(std::string("a\nb"))) == R"("a\nb")");
}

TEST_CASE("write_json правильно пишет вложенные arrays + objects", "[json][writer]") {
    lab4::Array grades;
    grades.emplace_back(4);
    grades.emplace_back(5);

    lab4::Object address;
    address.emplace_back("city", lab4::Value(std::string("Berdsk")));

    lab4::Object root;
    root.emplace_back("name", lab4::Value(std::string("Vasya")));
    root.emplace_back("grades", lab4::Value(std::move(grades)));
    root.emplace_back("address", lab4::Value(std::move(address)));

    REQUIRE(write_json_to_string(lab4::Value(std::move(root)))
      == R"({"name":"Vasya","grades":[4,5],"address":{"city":"Berdsk"}})");
}

TEST_CASE("write_json(parse_json(x)) == x (оно же известно как round-trip)", "[json][writer]") {
    std::string canonical = R"({"a":1,"b":[1,2,3],"c":{"d":true}})";
    REQUIRE(write_json_to_string(lab4::parse_json(canonical)) == canonical);
}

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
namespace {
    // json_text парсится JSON-парсером и сериализуется TOML-writer
    std::string json_to_toml(const std::string& json_text) {
        std::ostringstream out;
        lab4::write_toml(lab4::parse_json(json_text), out);

        return out.str();
    }
}// namespace

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

// ТЕСТЫ XML ПАРСЕРА ---------------------------------------------------------------------------------------------------

// ТЕСТЫ XML WRITER ----------------------------------------------------------------------------------------------------

// в самом конце сделать такой сценарий теста: JSON => TOML; TOML => XML; XML => JSON
// и сравнить изначальные данные и данные, полученные в конце.