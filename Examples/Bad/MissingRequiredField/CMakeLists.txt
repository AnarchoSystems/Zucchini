cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadMissingRequiredField LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(MissingRequiredFieldTest MissingRequiredField.h)
zucchinify(MissingRequiredFieldTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE MissingRequiredField
    DISCOVERY_MODE PRE_TEST)

expect_generator_failure(MissingRequiredField)