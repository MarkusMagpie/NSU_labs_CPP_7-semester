# add_subdirectory(catch2)
# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#usage
find_package(Catch2 REQUIRED)



add_executable(regex_tests test_regex.cpp)
target_link_libraries(regex_tests PRIVATE
    Catch2::Catch2
    regex_engine
)

include(CTest)
include(Catch)
catch_discover_tests(regex_tests)