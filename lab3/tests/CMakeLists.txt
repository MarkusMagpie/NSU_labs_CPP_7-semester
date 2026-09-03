# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#usage
# find_package(Catch2 REQUIRED)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Catch2 ${CMAKE_BINARY_DIR}/catch2_build)

add_executable(unit_tests test_regex.cpp)
target_link_libraries(unit_tests PRIVATE
    Catch2::Catch2WithMain
    regex_engine
)

# активация Ctest и интеграция с Catch
include(CTest)
include(Catch) # "Catch.cmake provides function catch_discover_tests to get tests from a target."
catch_discover_tests(unit_tests)

# CMake не знает какие именно тесты я написал внутри C++ файлов.
# Без этой строки пришлось бы вручную записывать каждый отдельный тест в файлы конфигурации CMake.
# include(Catch) = CMake сам находит тесты; регистрирует их CTest -> они будут выполнятся ctest утилитой
