# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#usage
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Catch2 ${CMAKE_BINARY_DIR}/catch2_build)

add_executable(unit_tests unit_tests.cpp ${CMAKE_SOURCE_DIR}/src/BankRounding.cpp)
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(unit_tests PRIVATE 
    Catch2::Catch2WithMain
)

# активация Ctest и интеграция с Catch
include(CTest)
include(${CMAKE_SOURCE_DIR}/third_party/Catch2/extras/Catch.cmake)
catch_discover_tests(unit_tests)

# CMake не знает какие именно тесты я написал внутри C++ файлов.
# Без этой строки пришлось бы вручную записывать каждый отдельный тест в файлы конфигурации CMake.
# include(Catch) = CMake сам находит тесты; регистрирует их CTest -> они будут выполнятся ctest утилитой
