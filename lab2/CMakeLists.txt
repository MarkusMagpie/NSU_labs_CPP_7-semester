cmake_minimum_required(VERSION 3.14)
project(lab2 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WITH_TESTS "Build unit tests (downloads Catch2 if not present)" OFF)

message(STATUS "source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "WITH_TESTS: ${WITH_TESTS}")

set(SOURCES
    src/main.cpp 
    src/BankRounding.cpp 
    src/DateTime.cpp 
    src/Money.cpp
    src/AccountTable.cpp
    src/DepositTable.cpp
    src/InputReader.cpp
    src/Enums.cpp
)

# консольное приложение моделирования банка
add_executable(bank 
    ${SOURCES}
)
target_include_directories(bank PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WITH_TESTS)
    # cmake переменная WITH_TESTS превращается в shell переменную
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
