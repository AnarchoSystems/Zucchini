cmake_minimum_required(VERSION 3.31)
project(ManifestUpdates LANGUAGES CXX)
enable_testing()
include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")
add_executable(ManifestUpdatesTest ManifestUpdates.h UpdateChecks.cpp)
zucchinify(ManifestUpdatesTest FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE ManifestUpdates DISCOVERY_MODE PRE_TEST)
zucchini_add_main(ManifestUpdatesTest ManifestUpdates)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/test-path-$<CONFIG>.cmake"
    CONTENT "set(UPDATE_TEST_EXECUTABLE [==[$<TARGET_FILE:ManifestUpdatesTest>]==])\n")
