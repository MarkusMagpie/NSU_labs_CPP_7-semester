find_package(GTest REQUIRED) # из conan: цели GTest::gtest, GTest::gtest_main

add_executable(libcompressortest libcompressortest.cpp)
target_link_libraries(libcompressortest PRIVATE libcompressor GTest::gtest_main)
# GTest::gtest_main уже содержит main() запускающий все тесты


# add_test регистрирует тест для ctest: https://cmake.org/cmake/help/latest/command/add_test.html
# CMAKE_BINARY_DIR = build/Release
# При выполнении тесты должны генерировать отчёт в формате xml по пути ${CMAKE_BINARY_DIR}/test_results.xml
add_test(NAME libcompressortest
        COMMAND libcompressortest --gtest_output=xml:${CMAKE_BINARY_DIR}/test_results.xml)