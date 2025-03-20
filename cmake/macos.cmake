set(CMAKE_PREFIX_PATH  "/Users/zhangteng/Workspace/runtime/macos/Qt/6.8.2/macos/lib/cmake")
set(DEPENDENCY_PATH "${OPC_CLIENT_RUNTIME_DIR}/macos")

find_package(Qt6 REQUIRED COMPONENTS Core)
set(CMAKE_AUTOMOC ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(spdlog REQUIRED)

find_package(yaml-cpp REQUIRED)

find_package(CppKafka REQUIRED)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${DEPENDENCY_PATH}/include
)

link_directories(
        ${DEPENDENCY_PATH}/lib
)

message("${DEPENDENCY_PATH}/lib")

add_executable(OPCClient
        src/main.cpp
        include/Machine.h
        src/Machine.cpp
        include/Logger.h
        src/Logger.cpp
        include/KafkaProducer.h
        src/KafkaProducer.cpp
        include/OPCClient.h
        src/OPCClient.cpp
        include/Exception.h
        src/Exception.cpp
        include/GlobalDefine.h
)

target_link_libraries(OPCClient
        Qt6::Core
        spdlog::spdlog
        yaml-cpp::yaml-cpp
        -lopen62541pp
        -lopen62541
        -lcppkafka
        -lrdkafka)

