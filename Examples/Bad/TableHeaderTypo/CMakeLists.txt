cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadTableHeaderTypo LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(TableHeaderTypoTest TableHeaderTypo.h)
zucchinify(TableHeaderTypoTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE TableHeaderTypo
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(TableHeaderTypoTest TableHeaderTypo)

expect_discovery_failure(TableHeaderTypoTest)