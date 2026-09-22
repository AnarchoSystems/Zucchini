cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadInvalidMethodName LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(InvalidMethodNameTest InvalidMethodName.h)
zucchinify(InvalidMethodNameTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE InvalidMethodName
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(InvalidMethodNameTest InvalidMethodName)

expect_generator_failure(InvalidMethodName)