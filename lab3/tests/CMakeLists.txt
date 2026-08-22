# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#usage
# find_package(Catch2 REQUIRED)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Catch2 ${CMAKE_BINARY_DIR}/catch2_build)

add_executable(unit_tests test_regex.cpp)
target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    regex_engine
)

include(CTest)
include(Catch)
catch_discover_tests(tests)