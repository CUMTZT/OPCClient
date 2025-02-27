cmake_minimum_required(VERSION 3.30)
project(OPCClient)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC ON)

find_package(spdlog REQUIRED)

find_package(yaml-cpp REQUIRED)

find_package(open62541 REQUIRED)

find_package(open62541pp REQUIRED)

find_package(CppKafka REQUIRED)

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
        src/OPCClientManager.cpp
        include/MessageProducer.h
        include/MessageConsumer.h
)

target_link_libraries(OPCClient
        Qt6::Core
        Qt6::Widgets
        spdlog::spdlog
        -lyaml-cpp
        open62541pp::open62541pp
        open62541::open62541
        -lcppkafka
        -lrdkafka)

