cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadInvalidScalarList LANGUAGES CXX)
include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")
add_executable(InvalidScalarListTest InvalidScalarList.h)
zucchinify(InvalidScalarListTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE InvalidScalarList DISCOVERY_MODE PRE_TEST)
zucchini_add_main(InvalidScalarListTest InvalidScalarList)
expect_discovery_failure(InvalidScalarListTest)
