cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadUserTags LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(UserTagsTest UserTags.h)
zucchinify(UserTagsTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE UserTags
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(UserTagsTest UserTags)

expect_discovery_failure(UserTagsTest)