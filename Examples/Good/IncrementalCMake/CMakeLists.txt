cmake_minimum_required(VERSION 3.27)
project(ZucchiniIncrementalCMake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${ZUCCHINIFY_MODULE_DIR}")
include(Zucchinify)

add_executable(Zucchini FakeGenerator.cpp)
add_library(LibZucchini INTERFACE)
add_library(Zucchini::LibZucchini ALIAS LibZucchini)
add_library(ZucchiniRuntime INTERFACE)
add_library(Zucchini::Runtime ALIAS ZucchiniRuntime)

add_executable(Probe Probe.cpp)
zucchinify(Probe
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Probe)

add_custom_command(TARGET Probe POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        -D "MARKER=${CMAKE_CURRENT_BINARY_DIR}/build.marker"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RecordBuild.cmake")