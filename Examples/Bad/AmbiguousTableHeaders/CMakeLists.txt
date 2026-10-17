cmake_minimum_required(VERSION 3.31)
project(ZucchiniBadAmbiguousTableHeaders LANGUAGES CXX)

include("${BAD_EXAMPLE_SUPPORT_DIR}/StandaloneZucchini.cmake")

add_executable(AmbiguousTableHeadersTest AmbiguousTableHeaders.h)
zucchinify(AmbiguousTableHeadersTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE AmbiguousTableHeaders
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(AmbiguousTableHeadersTest AmbiguousTableHeaders)

expect_discovery_failure(AmbiguousTableHeadersTest)