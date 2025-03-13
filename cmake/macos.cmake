set(CMAKE_PREFIX_PATH  "/Users/zhangteng/zhangteng/Workspace/runtime/macos/Qt/6.8.2/macos/lib/cmake")
set(DEPENDENCY_PATH "${OPC_CLIENT_RUNTIME_DIR}/macos")

find_package(Qt6 REQUIRED COMPONENTS Core)
set(CMAKE_AUTOMOC ON)

find_package(spdlog REQUIRED)

find_package(yaml-cpp REQUIRED)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${DEPENDENCY_PATH}/include
)

link_directories(
        ${DEPENDENCY_PATH}/lib
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

