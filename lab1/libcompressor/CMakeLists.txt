find_package(ZLIB REQUIRED) # создаст цель ZLIB::ZLIB
find_package(BZip2 REQUIRED) # BZip2::BZip2

add_library(libcompressor STATIC src/libcompressor.cpp)
# иначе liblibcompressor.a а по тз должно быть libcompressor.a
set_target_properties(libcompressor PROPERTIES OUTPUT_NAME compressor)

target_include_directories(libcompressor PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(libcompressor PRIVATE ZLIB::ZLIB
        BZip2::BZip2)

target_compile_features(libcompressor PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(libcompressor PRIVATE /W4 /WX)
else()
    target_compile_options(libcompressor PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# установка цели
install(TARGETS libcompressor
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


add_subdirectory(tests)