file(REMOVE_RECURSE "${TEST_BINARY_DIR}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${TEST_SOURCE_DIR}"
        -B "${TEST_BINARY_DIR}"
        -D "ZUCCHINI_EXECUTABLE=${ZUCCHINI_EXECUTABLE}"
        -D "BAD_EXAMPLE_SUPPORT_DIR=${BAD_EXAMPLE_SUPPORT_DIR}"
        -D "ZUCCHINI_PROJECT_SOURCE_DIR=${ZUCCHINI_PROJECT_SOURCE_DIR}"
        -D "ZUCCHINI_BUILD_DIR=${ZUCCHINI_BUILD_DIR}"
        -D "ZUCCHINI_LIBRARY=${ZUCCHINI_LIBRARY}"
        -D "CUCUMBER_GHERKIN_LIBRARY=${CUCUMBER_GHERKIN_LIBRARY}"
        -D "CUCUMBER_GHERKIN_INCLUDE_DIRS=${CUCUMBER_GHERKIN_INCLUDE_DIRS}"
        -D "CUCUMBER_MESSAGES_LIBRARY=${CUCUMBER_MESSAGES_LIBRARY}"
        -D "CUCUMBER_MESSAGES_INCLUDE_DIRS=${CUCUMBER_MESSAGES_INCLUDE_DIRS}"
        -D "GTEST_LIBRARY=${GTEST_LIBRARY}"
        -D "GTEST_INCLUDE_DIRS=${GTEST_INCLUDE_DIRS}"
        -D "JSON_INCLUDE_DIRS=${JSON_INCLUDE_DIRS}"
        -D "JSON_SCHEMA_VALIDATOR_LIBRARY=${JSON_SCHEMA_VALIDATOR_LIBRARY}"
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr)

if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR
        "Bad example failed before its declared stage (configure):\n"
        "${configure_stdout}${configure_stderr}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${TEST_BINARY_DIR}" --target failing-stage
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_stdout
    ERROR_VARIABLE build_stderr)

if(NOT build_result EQUAL 0)
    message(FATAL_ERROR
        "Bad example stage assertion failed:\n${build_stdout}${build_stderr}")
endif()