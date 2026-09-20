cmake_minimum_required(VERSION 3.27)
project(ZucchiniBadMalformedYaml LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(MalformedYamlTest MalformedYaml.h)
zucchinify(MalformedYamlTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE MalformedYaml
    DISCOVERY_MODE PRE_TEST)

expect_generator_failure(MalformedYaml)