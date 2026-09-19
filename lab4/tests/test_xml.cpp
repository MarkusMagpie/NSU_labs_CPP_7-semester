#include "test_helper.hpp"

// ТЕСТЫ XML ПАРСЕРА ---------------------------------------------------------------------------------------------------
TEST_CASE("parse_xml читает bool литералы", "[xml]") {
    lab4::Value v = lab4::parse_xml(R"(<root><entry key="a" type="bool">true</entry></root>)");
    REQUIRE(v.as_object()[0].second.as_bool() == true);
    lab4::Value v2 = lab4::parse_xml(R"(<root><entry key="a" type="bool">false</entry></root>)");
    REQUIRE(v2.as_object()[0].second.as_bool() == false);
}

TEST_CASE("parse_xml читает int", "[xml]") {
    lab4::Value v = lab4::parse_xml(
        R"(<root><entry key="a" type="int">0</entry><entry key="b" type="int">100</entry><entry key="c" type="int">-1000</entry></root>)");
    lab4::Object& obj = v.as_object(); // получу список из трех пар

    REQUIRE(obj[0].second.as_int() == 0);
    REQUIRE(obj[1].second.as_int() == 100);
    REQUIRE(obj[2].second.as_int() == -1000);
}

TEST_CASE("parse_xml читает double", "[xml]") {
    lab4::Value v = lab4::parse_xml(
        R"(<root><entry key="a" type="double">3.14</entry><entry key="b" type="double">-1.0e3</entry></root>)");
    lab4::Object& obj = v.as_object();

    REQUIRE(obj[0].second.as_double() == 3.14);
    REQUIRE(obj[1].second.as_double() == -1000.0);
}

TEST_CASE("parse_xml читает string", "[xml]") {
    lab4::Value v = lab4::parse_xml(R"(<root><entry key="a" type="string">hello</entry></root>)");
    REQUIRE(v.as_object()[0].second.as_string() == "hello");
}

TEST_CASE("parse_xml пропускает пролог перед корнем", "[xml]") {
    lab4::Value v = lab4::parse_xml(
        R"(<?xml version="1.0" encoding="UTF-8"?><root><entry key="a" type="int">1</entry></root>)");
    REQUIRE(v.as_object()[0].second.as_int() == 1);

    lab4::Value v2 = lab4::parse_xml(
        R"(<?xmlааа крокодилы бегемоты?><root><entry key="a" type="int">1</entry></root>)");
    REQUIRE(v2.as_object()[0].second.as_int() == 1);
}

TEST_CASE("parse_xml сохраняет порядок key атрибутов", "[xml]") {
    lab4::Value v = lab4::parse_xml(
        R"(<root><entry key="b" type="int">1</entry><entry key="a" type="int">2</entry></root>)");
    lab4::Object& obj = v.as_object();

    REQUIRE(obj.size() == 2);
    REQUIRE(obj[0].first == "b");
    REQUIRE(obj[1].first == "a");
}

TEST_CASE("parse_xml не зависит от порядка атрибутов key и type внутри entry", "[xml]") {
    lab4::Value v = lab4::parse_xml(R"(<root><entry type="int" key="a">1</entry></root>)");
    REQUIRE(v.as_object()[0].first == "a");
    REQUIRE(v.as_object()[0].second.as_int() == 1);
}

TEST_CASE("parse_xml обрабатывает какието ошибки", "[xml]") {
    REQUIRE_THROWS_AS(lab4::parse_xml(""), lab4::ParseError); // пустой документ, нет <root>
    REQUIRE_THROWS_AS(lab4::parse_xml(R"(<?xml version="1.0"><root></root>)"), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_xml(R"(<lalala></lalala>)"), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_xml(R"(<root>)"), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_xml(R"(<root></root> garbage)"), lab4::ParseError);

    REQUIRE_THROWS_AS(lab4::parse_xml(R"(<root><entry key="a">1</entry></root>)"), lab4::ParseError); // нет type
    REQUIRE_THROWS_AS(lab4::parse_xml(R"(<root><entry type="int">1</entry></root>)"), lab4::ParseError); // нет key

    REQUIRE_THROWS_AS(lab4::parse_xml(R"(<root><entry key="a" type="lalala">1</entry></root>)"), lab4::ParseError);
    REQUIRE_THROWS_AS(lab4::parse_xml(R"(<root><entry key="a" type="int">1</root>)"), lab4::ParseError);
}

// ТЕСТЫ XML WRITER (JSON -> XML) --------------------------------------------------------------------------------------
TEST_CASE("write_xml: json -> xml для скалярных значений", "[xml][writer]") {
    REQUIRE(json_to_xml(R"({"a": true, "b": 1, "c": 3.14, "d": "hello"})")
        == "<root>\n"
           "  <entry key=\"a\" type=\"bool\">true</entry>\n"
           "  <entry key=\"b\" type=\"int\">1</entry>\n"
           "  <entry key=\"c\" type=\"double\">3.14</entry>\n"
           "  <entry key=\"d\" type=\"string\">hello</entry>\n"
           "</root>\n");
}