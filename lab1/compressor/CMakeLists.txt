find_package(spdlog REQUIRED)  # цель spdlog::spdlog

add_executable(compressor src/compressor.cpp)
target_link_libraries(compressor PRIVATE libcompressor spdlog::spdlog)

# настройки сборки экзешника по ТЗ
target_compile_features(compressor PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(compressor PRIVATE /W4 /WX)
else()
    target_compile_options(compressor PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

install(TARGETS compressor
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})