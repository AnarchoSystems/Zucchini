include_guard(GLOBAL)

include(FetchContent)

# Resolves a dependency. Priority: local source dir > installed package > download.
macro(zucchini_dependency)
    set(_zdep_options)
    set(_zdep_one_value NAME PACKAGE REPO TAG SOURCE_SUBDIR)
    set(_zdep_multi_value FIND_PACKAGE_ARGS)
    cmake_parse_arguments(ZDEP "${_zdep_options}" "${_zdep_one_value}" "${_zdep_multi_value}" ${ARGN})

    string(TOUPPER "${ZDEP_NAME}" _zdep_upper)

    option(ZUCCHINI_FETCH_${_zdep_upper}
        "Download ${ZDEP_NAME} when it is not already available."
        ${ZUCCHINI_FETCH_DEPENDENCIES})
    set(ZUCCHINI_${_zdep_upper}_SOURCE_DIR ""
        CACHE PATH "Local checkout of ${ZDEP_NAME}; takes precedence over installed and downloaded copies.")
    set(ZUCCHINI_${_zdep_upper}_GIT_REPOSITORY "${ZDEP_REPO}"
        CACHE STRING "Git repository used to download ${ZDEP_NAME}.")
    set(ZUCCHINI_${_zdep_upper}_GIT_TAG "${ZDEP_TAG}"
        CACHE STRING "Git tag used to download ${ZDEP_NAME}.")

    set(_zdep_existing_target "")
    if(ZDEP_NAME STREQUAL "nlohmann_json" AND TARGET nlohmann_json::nlohmann_json)
        set(_zdep_existing_target nlohmann_json::nlohmann_json)
    elseif(ZDEP_NAME STREQUAL "googletest" AND TARGET GTest::gtest)
        set(_zdep_existing_target GTest::gtest)
    endif()

    if(_zdep_existing_target)
        message(STATUS "Zucchini: using existing target ${_zdep_existing_target} for ${ZDEP_NAME}")
    elseif(ZUCCHINI_${_zdep_upper}_SOURCE_DIR)
        set(_zdep_local "${ZUCCHINI_${_zdep_upper}_SOURCE_DIR}")
        set(_zdep_binary_dir "${CMAKE_BINARY_DIR}/_deps/${ZDEP_NAME}-local")
        if(ZDEP_SOURCE_SUBDIR)
            set(_zdep_local "${_zdep_local}/${ZDEP_SOURCE_SUBDIR}")
        endif()
        message(STATUS "Zucchini: using local ${ZDEP_NAME} from ${_zdep_local}")
        add_subdirectory("${_zdep_local}" "${_zdep_binary_dir}" EXCLUDE_FROM_ALL)
        list(APPEND CMAKE_PREFIX_PATH "${_zdep_binary_dir}")
        unset(_zdep_local)
    elseif(NOT ZUCCHINI_FETCH_${_zdep_upper})
        find_package(${ZDEP_PACKAGE} CONFIG REQUIRED)
    else()
        FetchContent_Declare(${ZDEP_NAME}
            GIT_REPOSITORY "${ZUCCHINI_${_zdep_upper}_GIT_REPOSITORY}"
            GIT_TAG "${ZUCCHINI_${_zdep_upper}_GIT_TAG}"
            GIT_SHALLOW TRUE
            SOURCE_SUBDIR "${ZDEP_SOURCE_SUBDIR}"
            FIND_PACKAGE_ARGS ${ZDEP_FIND_PACKAGE_ARGS})
        FetchContent_MakeAvailable(${ZDEP_NAME})
        list(APPEND CMAKE_PREFIX_PATH "${${ZDEP_NAME}_BINARY_DIR}")
    endif()

    unset(_zdep_binary_dir)
    unset(_zdep_existing_target)
    unset(_zdep_upper)
endmacro()

# The cucumber libraries export targets that link nlohmann_json, so it must be installable too.
set(JSON_Install ON CACHE BOOL "" FORCE)

zucchini_dependency(
    NAME nlohmann_json
    PACKAGE nlohmann_json
    REPO https://github.com/nlohmann/json
    TAG v3.12.0
    FIND_PACKAGE_ARGS 3.12 CONFIG)

set(JSON_VALIDATOR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_INSTALL OFF CACHE BOOL "" FORCE)
set(nlohmann_json_VERSION 3.12.0 CACHE STRING "Version of nlohmann_json to use")
zucchini_dependency(
    NAME nlohmann_json_schema_validator
    PACKAGE nlohmann_json_schema_validator
    REPO https://github.com/pboettch/json-schema-validator
    TAG 2.3.0
    FIND_PACKAGE_ARGS CONFIG)

zucchini_dependency(
    NAME fkYAML
    PACKAGE fkYAML
    REPO https://github.com/fktn-k/fkYAML
    TAG v0.5.0
    FIND_PACKAGE_ARGS CONFIG)

# Cucumber Gherkin brings in the matching Cucumber Messages package.  Its
# build-tree package config is not complete until installation, so resolve
# that pair through Gherkin's supported FetchContent path.
set(CUCUMBER_GHERKIN_FETCH_DEPS ON CACHE BOOL "" FORCE)
zucchini_dependency(
    NAME cucumber_gherkin
    PACKAGE cucumber_gherkin
    REPO https://github.com/cucumber/gherkin
    TAG v42.0.1
    SOURCE_SUBDIR cpp
    FIND_PACKAGE_ARGS CONFIG)

# googletest is used by ZucchiniRuntime and the repository tests.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
zucchini_dependency(
    NAME googletest
    PACKAGE GTest
    REPO https://github.com/google/googletest
    TAG v1.15.2
    FIND_PACKAGE_ARGS NAMES GTest)
