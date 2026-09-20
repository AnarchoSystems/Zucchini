cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadUnanchoredStepRegex LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(UnanchoredStepRegexTest UnanchoredStepRegex.h)
zucchinify(UnanchoredStepRegexTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE UnanchoredStepRegex
    DISCOVERY_MODE PRE_TEST)

expect_generator_failure(UnanchoredStepRegex)