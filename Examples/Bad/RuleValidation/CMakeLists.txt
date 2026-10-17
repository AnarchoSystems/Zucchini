cmake_minimum_required(VERSION 3.31)
project(ZucchiniBadRuleValidation LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(RuleValidationTest RuleValidation.h)
zucchinify(RuleValidationTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE RuleValidation
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(RuleValidationTest RuleValidation)

expect_discovery_failure(RuleValidationTest)