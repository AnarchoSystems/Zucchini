cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadMalformedTable LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(MalformedTableTest MalformedTable.h)
zucchinify(MalformedTableTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE MalformedTable
    DISCOVERY_MODE PRE_TEST)

expect_discovery_failure(MalformedTableTest)