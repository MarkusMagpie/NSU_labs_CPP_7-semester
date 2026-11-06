cmake_minimum_required(VERSION 3.20)
project(lab7_broker CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra) # можно будет убрать

# поиск зависимостей 
find_package(Threads REQUIRED) # pthread
find_package(nlohmann_json REQUIRED) # json
find_package(yaml-cpp REQUIRED) # yaml
# yaml-cpp в разных версиях экспортирует разные имена таргетов
if(TARGET yaml-cpp::yaml-cpp)
  set(LAB7_YAML_TARGET yaml-cpp::yaml-cpp)
else()
  set(LAB7_YAML_TARGET yaml-cpp)
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED IMPORTED_TARGET libzmq) # PkgConfig::ZMQ

# общие/common утилиты: логгер, конфиг
add_library(lab7_common STATIC
  src/common/Logger.cpp
  src/common/YamlConfig.cpp
)
target_include_directories(lab7_common PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(lab7_common PUBLIC
  nlohmann_json::nlohmann_json
  ${LAB7_YAML_TARGET}
)

add_library(lab7_protocol STATIC
  src/protocol/Envelope.cpp
)
target_include_directories(lab7_protocol PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(lab7_protocol PUBLIC 
  lab7_common
)

# клиентская библиотека
add_library(lab7_client STATIC
  src/client/BrokerClient.cpp
)
target_include_directories(lab7_client PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(lab7_client PUBLIC
  PkgConfig::ZMQ
)

# для проверки связи с брокером
add_executable(test_client src/test_client.cpp)
target_link_libraries(test_client PRIVATE lab7_client lab7_protocol)

add_executable(broker src/broker_main.cpp)
target_link_libraries(broker PRIVATE
  lab7_protocol
  PkgConfig::ZMQ
  Threads::Threads
)
