cmake_minimum_required(VERSION 3.20)
project(compressor CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES) # без compile_commands.json не запустится clang-format и clang-tidy
include(GNUInstallDirs) # CMAKE_INSTALL_INCLUDEDIR, CMAKE_INSTALL_LIBDIR не пустые

enable_testing()

add_subdirectory(libcompressor)
add_subdirectory(compressor)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    set (DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
    set (DOXYGEN_EXTRACT_ALL YES)
    # https://cmake.org/cmake/help/latest/module/FindDoxygen.html#command:doxygen_add_docs
    doxygen_add_docs(docs ${CMAKE_SOURCE_DIR}/libcompressor/include)
endif ()