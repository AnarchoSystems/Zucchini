cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadInvalidEnumValue LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(InvalidEnumValueTest InvalidEnumValue.h)
zucchinify(InvalidEnumValueTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE InvalidEnumValue
    DISCOVERY_MODE PRE_TEST)

expect_discovery_failure(InvalidEnumValueTest)