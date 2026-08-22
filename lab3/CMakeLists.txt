cmake_minimum_required(VERSION 3.14)
project(lab3_regex CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WITH_TESTS "Build unit tests (downloads Catch2 if not present)" OFF)

message(STATUS "source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "WITH_TESTS: ${WITH_TESTS}")

add_library(regex_engine STATIC regex_engine.cpp)
target_include_directories(regex_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

add_executable(res main.cpp)
target_link_libraries(res PRIVATE regex_engine)

if(WITH_TESTS)
    # cmake переменная превращается в shell переменную
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env WITH_TESTS=1
                ${CMAKE_SOURCE_DIR}/get_dependencies.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE RESULT
    )

    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "get_dependencies.sh failed")
    endif()

    enable_testing()
    add_subdirectory(tests)
endif()