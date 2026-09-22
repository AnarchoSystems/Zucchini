cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadInvalidRegexArgument LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(InvalidRegexArgumentTest InvalidRegexArgument.h)
zucchinify(InvalidRegexArgumentTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE InvalidRegexArgument
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(InvalidRegexArgumentTest InvalidRegexArgument)

expect_discovery_failure(InvalidRegexArgumentTest)