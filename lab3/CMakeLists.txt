cmake_minimum_required(VERSION 3.14)
project(lab3_regex CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "source directoryy: ${CMAKE_SOURCE_DIR}")

add_library(regex_engine STATIC regex_engine.cpp) # libregex_engine.a в build
target_include_directories(regex_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

add_executable(res main.cpp)
target_link_libraries(res PRIVATE regex_engine)

enable_testing() # ctest --test-dir build
add_subdirectory(tests)