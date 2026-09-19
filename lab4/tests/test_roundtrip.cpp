#include "test_helper.hpp"

TEST_CASE("JSON => TOML => XML => JSON: скаляры переживают полный цикл", "[roundtrip]") {
    std::string canon = R"({"a100500":true,"b123":1,"c4":3.14,"d13":"hello"})";
    REQUIRE(json_via_toml_via_xml(canon) == canon);
}