if(NOT ZUCCHINI_PROJECT_SOURCE_DIR)
    message(FATAL_ERROR "ZUCCHINI_PROJECT_SOURCE_DIR must name the Zucchini source tree")
endif()
if(NOT EXISTS "${ZUCCHINI_EXECUTABLE}" OR NOT EXISTS "${ZUCCHINI_LIBRARY}")
    message(FATAL_ERROR "Build Zucchini before configuring a standalone bad example")
endif()

add_executable(Zucchini IMPORTED GLOBAL)
set_target_properties(Zucchini PROPERTIES IMPORTED_LOCATION "${ZUCCHINI_EXECUTABLE}")

add_library(ZucchiniLib STATIC IMPORTED GLOBAL)
add_library(Zucchini::LibZucchini ALIAS ZucchiniLib)
set_target_properties(ZucchiniLib PROPERTIES
    IMPORTED_LOCATION "${ZUCCHINI_LIBRARY}"
    INTERFACE_COMPILE_FEATURES cxx_std_17
    INTERFACE_INCLUDE_DIRECTORIES
        "${ZUCCHINI_PROJECT_SOURCE_DIR}/LibZucchini/include;${CUCUMBER_GHERKIN_INCLUDE_DIRS};${CUCUMBER_MESSAGES_INCLUDE_DIRS};${JSON_INCLUDE_DIRS};${GTEST_INCLUDE_DIRS};${JSON_SCHEMA_VALIDATOR_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
        "${CUCUMBER_GHERKIN_LIBRARY};${CUCUMBER_MESSAGES_LIBRARY};${GTEST_LIBRARY};${JSON_SCHEMA_VALIDATOR_LIBRARY}")

list(APPEND CMAKE_MODULE_PATH "${ZUCCHINI_PROJECT_SOURCE_DIR}/cmake")
include(Zucchinify)

function(expect_generator_failure fixture)
    add_custom_target(failing-stage
        COMMAND "${CMAKE_COMMAND}"
            -D "ZUCCHINI_EXECUTABLE=${ZUCCHINI_EXECUTABLE}"
            -D "CASE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -D "EXPECTED_STDERR=${CMAKE_CURRENT_SOURCE_DIR}/expected.stderr"
            -D "FIXTURE=${fixture}"
            -P "${BAD_EXAMPLE_SUPPORT_DIR}/AssertGeneratorFailure.cmake"
        VERBATIM)
endfunction()

function(expect_discovery_failure target)
    add_custom_target(failing-stage
        COMMAND "${CMAKE_COMMAND}"
            -D "TEST_EXECUTABLE=$<TARGET_FILE:${target}>"
            -D "CASE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -D "EXPECTED_STDERR=${CMAKE_CURRENT_SOURCE_DIR}/expected.stderr"
            -P "${BAD_EXAMPLE_SUPPORT_DIR}/AssertDiscoveryFailure.cmake"
        DEPENDS ${target}
        VERBATIM)
endfunction()

function(expect_test_failure target)
    add_custom_target(failing-stage
        COMMAND "${CMAKE_COMMAND}"
            -D "TEST_EXECUTABLE=$<TARGET_FILE:${target}>"
            -D "CASE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${BAD_EXAMPLE_SUPPORT_DIR}/AssertTestFailure.cmake"
        DEPENDS ${target}
        VERBATIM)
endfunction()