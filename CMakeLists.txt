cmake_minimum_required(VERSION 3.30)
project(OPCClient)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core)
set(CMAKE_AUTOMOC ON)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(OPCClient
        include/OPCClient.h
        src/main.cpp
        src/OPCClient.cpp
        include/Logger.h
        src/Logger.cpp
        include/KafkaProduder.h
        src/KafkaProduder.cpp)

target_link_libraries(OPCClient
        Qt6::Core
        -lopen62541
        -lyaml-cpp
        -lcppkafka)

