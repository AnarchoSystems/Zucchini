cmake_minimum_required(VERSION 3.24)
project(Zucchini LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ZUCCHINI_IS_TOP_LEVEL ON)
else()
    set(ZUCCHINI_IS_TOP_LEVEL OFF)
endif()

option(ZUCCHINI_BUILD_TESTS "Build the Zucchini test targets." ${ZUCCHINI_IS_TOP_LEVEL})
option(ZUCCHINI_FETCH_DEPENDENCIES "Download missing dependencies instead of requiring them to be installed." ON)

include(Dependencies)

if(ZUCCHINI_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(LibZucchini)

