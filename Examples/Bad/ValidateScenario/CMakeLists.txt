cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadValidateScenario LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(ValidateScenarioTest ValidateScenario.h)
zucchinify(ValidateScenarioTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE ValidateScenario
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(ValidateScenarioTest ValidateScenario)

expect_discovery_failure(ValidateScenarioTest)