#include <catch2/catch_test_macros.hpp>

#include <BankRounding.hpp>

// подключение Catch2 и CTest
TEST_CASE("Catch2 работает") {
    REQUIRE(2 + 2 == 4);
}

/* ----- Банковское округление -----*/
TEST_CASE("Округление до ближайшего целого") {
    REQUIRE(bankRound(0.0) == 0.0);
    REQUIRE(bankRound(2.25) == 2.0);
    REQUIRE(bankRound(2.75) == 3.0);
    REQUIRE(bankRound(-2.25) == -2.0);
    REQUIRE(bankRound(-2.75) == -3.0);
}

TEST_CASE("Половина округляется к четному целому") {
    REQUIRE(bankRound(0.5) == 0.0);
    REQUIRE(bankRound(1.5) == 2.0);
    REQUIRE(bankRound(2.5) == 2.0);
    REQUIRE(bankRound(3.5) == 4.0);
    REQUIRE(bankRound(-2.5) == -2.0);
    REQUIRE(bankRound(-3.5) == -4.0);
}
/* ---------------------------------*/
