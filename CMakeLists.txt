cmake_minimum_required(VERSION 3.27)
project(Zucchini LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Without this, MSVC decodes narrow string/UCN literals (e.g. Naming.cpp's
    # transliteration table) using the active code page instead of UTF-8,
    # silently corrupting the multi-byte sequences the code operates on.
    add_compile_options(/utf-8)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ZUCCHINI_IS_TOP_LEVEL ON)
else()
    set(ZUCCHINI_IS_TOP_LEVEL OFF)
endif()

option(ZUCCHINI_BUILD_TESTS "Build the Zucchini test targets." ${ZUCCHINI_IS_TOP_LEVEL})
option(ZUCCHINI_FETCH_DEPENDENCIES "Download missing dependencies instead of requiring them to be installed." ON)

include(Dependencies)
include(TppTools)
include(Zucchinify)

if(ZUCCHINI_BUILD_TESTS)
    enable_testing()
    add_test(NAME control-flow-brace-check
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_control_flow_braces.sh
            ${CMAKE_CURRENT_SOURCE_DIR}/LibZucchini
            ${CMAKE_CURRENT_SOURCE_DIR}/Zucchini
            ${CMAKE_CURRENT_SOURCE_DIR}/ZucchiniRuntime
            ${CMAKE_CURRENT_SOURCE_DIR}/Examples)
    set_tests_properties(control-flow-brace-check PROPERTIES
        PASS_REGULAR_EXPRESSION "control-flow brace enforcement passed")
endif()

add_subdirectory(LibZucchini)
add_subdirectory(ZucchiniRuntime)
add_subdirectory(Zucchini)

if(ZUCCHINI_BUILD_TESTS)
    add_subdirectory(Examples)
endif()

