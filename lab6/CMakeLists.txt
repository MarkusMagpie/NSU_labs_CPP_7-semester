cmake_minimum_required(VERSION 3.14)
project(lab6_warehouse CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "source directory: ${CMAKE_SOURCE_DIR}")

# библиотека потоков (на Linux pthread)
find_package(Threads REQUIRED)

set(SOURCES
    src/main.cpp
    src/utils/RandomGenerator.cpp
    src/utils/Config.cpp
    src/model/ProductType.cpp
    src/model/Pallet.cpp
    src/model/Truck.cpp
    src/model/StorageAddress.cpp
    src/model/Order.cpp
)

add_executable(lab6 ${SOURCES})

target_include_directories(lab6 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(lab6 PRIVATE Threads::Threads)

target_compile_options(lab6 PRIVATE -Wall -Wextra)
