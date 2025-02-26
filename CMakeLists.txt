cmake_minimum_required(VERSION 3.30)
project(OPCClient)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core)
set(CMAKE_AUTOMOC ON)

find_package(spdlog REQUIRED)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(OPCClient
        include/OPCClient.h
        src/main.cpp
        src/OPCClient.cpp
        include/Logger.h
        src/Logger.cpp
        include/KafkaProducer.h
        src/KafkaProducer.cpp
        include/OPCClientManager.h
        src/OPCClientManager.cpp)

target_link_libraries(OPCClient
        Qt6::Core
        spdlog::spdlog
        -lopen62541pp
        -lopen62541
        -lyaml-cpp
        -lcppkafka)

