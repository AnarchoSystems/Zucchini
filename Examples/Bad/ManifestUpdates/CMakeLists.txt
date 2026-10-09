cmake_minimum_required(VERSION 3.27)
project(ManifestUpdates LANGUAGES CXX)
enable_testing()
include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")
add_executable(ManifestUpdatesTest ManifestUpdates.h)
zucchinify(ManifestUpdatesTest FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE ManifestUpdates DISCOVERY_MODE PRE_TEST)
zucchini_add_main(ManifestUpdatesTest ManifestUpdates)
