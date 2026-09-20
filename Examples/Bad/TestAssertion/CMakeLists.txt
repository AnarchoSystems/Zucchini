cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadTestAssertion LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(TestAssertionTest TestAssertion.h)
zucchinify(TestAssertionTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE TestAssertion
    DISCOVERY_MODE PRE_TEST)

expect_test_failure(TestAssertionTest)