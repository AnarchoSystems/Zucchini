cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadWrongTypedDocStringValue LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(WrongTypedDocStringValueTest WrongTypedDocStringValue.h)
zucchinify(WrongTypedDocStringValueTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE WrongTypedDocStringValue
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(WrongTypedDocStringValueTest WrongTypedDocStringValue)

expect_discovery_failure(WrongTypedDocStringValueTest)