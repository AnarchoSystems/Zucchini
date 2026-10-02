cmake_minimum_required(VERSION 3.27)
project(ZucchiniPreTestDiscovery LANGUAGES CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

foreach(required IN ITEMS ZUCCHINI_PROJECT_SOURCE_DIR ZUCCHINI_EXECUTABLE
        ZUCCHINI_LIBRARY ZUCCHINI_RUNTIME_LIBRARY CUCUMBER_GHERKIN_LIBRARY
        CUCUMBER_MESSAGES_LIBRARY GTEST_LIBRARY GTEST_INCLUDE_DIRS
        CUCUMBER_GHERKIN_INCLUDE_DIRS CUCUMBER_MESSAGES_INCLUDE_DIRS
        JSON_INCLUDE_DIRS)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} must be provided by the parent build")
    endif()
endforeach()

add_executable(Zucchini IMPORTED GLOBAL)
set_target_properties(Zucchini PROPERTIES IMPORTED_LOCATION "${ZUCCHINI_EXECUTABLE}")

add_library(GTest::gtest UNKNOWN IMPORTED GLOBAL)
set_target_properties(GTest::gtest PROPERTIES
    IMPORTED_LOCATION "${GTEST_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIRS}")

add_library(ZucchiniLib STATIC IMPORTED GLOBAL)
add_library(Zucchini::LibZucchini ALIAS ZucchiniLib)
set_target_properties(ZucchiniLib PROPERTIES
    IMPORTED_LOCATION "${ZUCCHINI_LIBRARY}"
    INTERFACE_COMPILE_FEATURES cxx_std_17
    INTERFACE_INCLUDE_DIRECTORIES "${ZUCCHINI_PROJECT_SOURCE_DIR}/LibZucchini/include")

add_library(ZucchiniRuntime STATIC IMPORTED GLOBAL)
add_library(Zucchini::Runtime ALIAS ZucchiniRuntime)
set_target_properties(ZucchiniRuntime PROPERTIES
    IMPORTED_LOCATION "${ZUCCHINI_RUNTIME_LIBRARY}"
    INTERFACE_COMPILE_FEATURES cxx_std_17
    INTERFACE_INCLUDE_DIRECTORIES
        "${ZUCCHINI_PROJECT_SOURCE_DIR}/ZucchiniRuntime/include;${ZUCCHINI_PROJECT_SOURCE_DIR}/LibZucchini/include;${CUCUMBER_MESSAGES_INCLUDE_DIRS};${JSON_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
        "Zucchini::LibZucchini;${CUCUMBER_GHERKIN_LIBRARY};${CUCUMBER_MESSAGES_LIBRARY}")

list(APPEND CMAKE_MODULE_PATH "${ZUCCHINI_PROJECT_SOURCE_DIR}/cmake")
include(Zucchinify)

add_executable(PreTestDiscoveryTest)
zucchinify(PreTestDiscoveryTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE PreTestDiscovery
    DISCOVERY_MODE PRE_TEST)
zucchini_add_main(PreTestDiscoveryTest PreTestDiscovery)
