cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadMalformedTypedDocString LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(MalformedTypedDocStringTest MalformedTypedDocString.h)
zucchinify(MalformedTypedDocStringTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE MalformedTypedDocString
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(MalformedTypedDocStringTest MalformedTypedDocString)

expect_discovery_failure(MalformedTypedDocStringTest)