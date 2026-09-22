cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadCppKeywordIdentifier LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(CppKeywordIdentifierTest CppKeywordIdentifier.h)
zucchinify(CppKeywordIdentifierTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE CppKeywordIdentifier
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(CppKeywordIdentifierTest CppKeywordIdentifier)

expect_generator_failure(CppKeywordIdentifier)