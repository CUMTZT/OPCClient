cmake_minimum_required(VERSION 3.25)
project(OPCClient)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(OPC_CLIENT_RUNTIME_DIR "${PROJECT_SOURCE_DIR}/../runtime")

if(CMAKE_HOST_APPLE)
    message("Current Platform is MacOS")
    include(cmake/macos.cmake)
elseif (CMAKE_HOST_LINUX)
    message("Current Platform is Linux")
    include(cmake/linux.cmake)
else ()
    message(FATAL_ERROR "Current Platform do not Support: ${CMAKE_SYSTEM_NAME}!")
endif ()
