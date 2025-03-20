set(CMAKE_PREFIX_PATH  "${OPC_CLIENT_RUNTIME_DIR}/linux/x86_64/Qt/6.8.2/gcc_64/lib/cmake")
set(DEPENDENCY_PATH "${OPC_CLIENT_RUNTIME_DIR}/linux/x86_64/")

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

find_package(spdlog REQUIRED)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${DEPENDENCY_PATH}/include
)

link_directories(
        ${DEPENDENCY_PATH}/lib
)

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
        -lyaml-cpp
        -lopen62541pp
        -lopen62541
        -lcppkafka
        -lrdkafka
        )

