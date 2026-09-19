# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#usage
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Catch2 ${CMAKE_BINARY_DIR}/catch2_build)

add_executable(
        unit_tests

        test_format.cpp
        test_json.cpp
        test_toml.cpp
        test_xml.cpp
        test_roundtrip.cpp
        # рудиментарный экзешник (после разбиения на тестовые файлы не существует) 19.09.26
        # test_lab4.cpp
)
target_link_libraries(unit_tests PRIVATE
    Catch2::Catch2WithMain
    tree
)

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
