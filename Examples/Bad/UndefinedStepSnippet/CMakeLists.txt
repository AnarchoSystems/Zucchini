cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadUndefinedStepSnippet LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(UndefinedStepSnippetTest UndefinedStepSnippet.h)
zucchinify(UndefinedStepSnippetTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE UndefinedStepSnippet
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(UndefinedStepSnippetTest UndefinedStepSnippet)

expect_discovery_failure(UndefinedStepSnippetTest)
