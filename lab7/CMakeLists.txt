cmake_minimum_required(VERSION 3.20)
project(lab7_broker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra) # можно будет убрать

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(FETCH_DEPENDENCIES "Скачать и собрать зависимости в third_party" OFF)
if(FETCH_DEPENDENCIES)
  set(LAB7_DEPS_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/third_party/install" CACHE PATH "префикс зависимостей lab7")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E env WITH_LAB7_DEPS=1 LAB7_DEPS_PREFIX=${LAB7_DEPS_PREFIX}
            bash ${CMAKE_CURRENT_SOURCE_DIR}/get_dependencies.sh
    RESULT_VARIABLE RESULT
  )

  if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "get_dependencies.sh failed")
  endif()

  list(PREPEND CMAKE_PREFIX_PATH "${LAB7_DEPS_PREFIX}") # каталог с зависимостями в начало путей поиска CMake для find_package
endif()

option(BUILD_BROKER "Собирать брокер" ON)



find_package(nlohmann_json REQUIRED) # json
find_package(cppzmq CONFIG REQUIRED) # + libzmq
if(BUILD_BROKER)
find_package(Threads REQUIRED) # pthread
find_package(yaml-cpp REQUIRED) # yaml

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
  yaml-cpp::yaml-cpp
)
endif()

add_library(lab7_protocol STATIC
  src/protocol/Envelope.cpp
)
target_include_directories(lab7_protocol PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(lab7_protocol PUBLIC 
  nlohmann_json::nlohmann_json
)
target_compile_features(lab7_protocol PUBLIC cxx_std_20)

# очереди и хранение сообщений брокера
if(BUILD_BROKER)
add_library(lab7_broker_core STATIC
  src/broker/Queue.cpp
  src/broker/QueueManager.cpp
  src/broker/DiskStorage.cpp
)
target_include_directories(lab7_broker_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(lab7_broker_core PUBLIC
  lab7_common
  Threads::Threads
)
endif()

add_library(lab7_client STATIC
  src/client/BrokerClient.cpp
)
target_include_directories(lab7_client PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(lab7_client PUBLIC
  cppzmq
)
target_compile_features(lab7_client PUBLIC cxx_std_20)

# имена целей после установки другим разработчиком
set_target_properties(lab7_client PROPERTIES EXPORT_NAME Client) # Lab7::Client
set_target_properties(lab7_protocol PROPERTIES EXPORT_NAME Protocol) # Lab7::Protocol
# cmake --install - установка библиотек и заголовков в группу экспорта
install(TARGETS lab7_client lab7_protocol 
  EXPORT Lab7ClientTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  # LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/client include/protocol 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# куда установить CMake файлы пакета
set(LAB7_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/Lab7Client") 
# экспорт группы экспорта: Lab7ClientTargets -> Lab7ClientTargets.cmake 
install(EXPORT Lab7ClientTargets 
  FILE Lab7ClientTargets.cmake 
  NAMESPACE Lab7::
  DESTINATION ${LAB7_CMAKE_INSTALL_DIR}
)
# cmake/Lab7ClientConfig.cmake.in -> Lab7ClientConfig.cmake
configure_package_config_file(
  cmake/Lab7ClientConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/Lab7ClientConfig.cmake"
  INSTALL_DESTINATION ${LAB7_CMAKE_INSTALL_DIR}
)
install(FILES 
  "${CMAKE_CURRENT_BINARY_DIR}/Lab7ClientConfig.cmake"
  DESTINATION ${LAB7_CMAKE_INSTALL_DIR}
)

# проверка 1 связи с брокером
# add_executable(test_client src/test_client.cpp)
# target_link_libraries(test_client PRIVATE lab7_client lab7_protocol)

# проверка 2 сценария 1 производитель и 2 потребителя с получением ими сообщений 
# add_executable(m2_scenario src/m2_scenario.cpp)
# target_link_libraries(m2_scenario PRIVATE lab7_client lab7_protocol)

# проверка 3 выдачи sorted-очереди потребителям with_ack
# add_executable(m4_with_ack_scenario src/m4_with_ack_scenario.cpp)
# target_link_libraries(m4_with_ack_scenario PRIVATE lab7_client lab7_protocol Threads::Threads)

# восстановление дисковой очереди
add_executable(disk_scenario EXCLUDE_FROM_ALL src/disk_scenario.cpp)
target_link_libraries(disk_scenario PRIVATE lab7_client lab7_protocol)

if(BUILD_BROKER)
add_executable(broker src/broker_main.cpp)
target_link_libraries(broker PRIVATE
  lab7_broker_core
  lab7_protocol
  cppzmq
  Threads::Threads
)
endif()
