cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadMissingRequiredTableField LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(MissingRequiredTableFieldTest MissingRequiredTableField.h)
zucchinify(MissingRequiredTableFieldTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE MissingRequiredTableField
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(MissingRequiredTableFieldTest MissingRequiredTableField)

expect_discovery_failure(MissingRequiredTableFieldTest)