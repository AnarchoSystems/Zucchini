cmake_minimum_required(VERSION 3.31)
project(ZucchiniBadPrintStepSnippet LANGUAGES CXX)
enable_testing()
include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(PrintStepSnippetTest PrintStepSnippet.h)
zucchinify(PrintStepSnippetTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE PrintStepSnippet
    DISCOVERY_MODE PRE_TEST
    SNIPPET_MODE PRINT)
zucchini_add_main(PrintStepSnippetTest PrintStepSnippet)

add_custom_target(failing-stage
    COMMAND "${CMAKE_COMMAND}"
        -D "CASE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        -D "CASE_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -D "CTEST_EXECUTABLE=${CMAKE_CTEST_COMMAND}"
        -D "TEST_CONFIG=$<CONFIG>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/AssertPrint.cmake"
    DEPENDS PrintStepSnippetTest VERBATIM)
