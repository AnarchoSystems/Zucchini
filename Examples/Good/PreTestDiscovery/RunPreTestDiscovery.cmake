file(REMOVE_RECURSE "${TEST_BINARY_DIR}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${TEST_SOURCE_DIR}"
        -B "${TEST_BINARY_DIR}"
        -D "CMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}"
        -D "CMAKE_CXX_FLAGS=${TEST_CXX_FLAGS}"
        -D "ZUCCHINI_PROJECT_SOURCE_DIR=${ZUCCHINI_PROJECT_SOURCE_DIR}"
        -D "ZUCCHINI_EXECUTABLE=${ZUCCHINI_EXECUTABLE}"
        -D "ZUCCHINI_LIBRARY=${ZUCCHINI_LIBRARY}"
        -D "ZUCCHINI_RUNTIME_LIBRARY=${ZUCCHINI_RUNTIME_LIBRARY}"
        -D "CUCUMBER_GHERKIN_LIBRARY=${CUCUMBER_GHERKIN_LIBRARY}"
        -D "CUCUMBER_GHERKIN_INCLUDE_DIRS=${CUCUMBER_GHERKIN_INCLUDE_DIRS}"
        -D "CUCUMBER_MESSAGES_LIBRARY=${CUCUMBER_MESSAGES_LIBRARY}"
        -D "CUCUMBER_MESSAGES_INCLUDE_DIRS=${CUCUMBER_MESSAGES_INCLUDE_DIRS}"
        -D "GTEST_LIBRARY=${GTEST_LIBRARY}"
        -D "GTEST_INCLUDE_DIRS=${GTEST_INCLUDE_DIRS}"
        -D "JSON_INCLUDE_DIRS=${JSON_INCLUDE_DIRS}"
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR
        "PRE_TEST example failed to configure:\n${configure_stdout}${configure_stderr}")
endif()

set(build_command "${CMAKE_COMMAND}" --build "${TEST_BINARY_DIR}")
if(TEST_BUILD_CONFIG)
    list(APPEND build_command --config "${TEST_BUILD_CONFIG}")
endif()
list(APPEND build_command --target PreTestDiscoveryTest)
execute_process(
    COMMAND ${build_command}
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_stdout
    ERROR_VARIABLE build_stderr)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR
        "PRE_TEST example failed to build:\n${build_stdout}${build_stderr}")
endif()

set(plan_dir "${TEST_BINARY_DIR}/zucchini-manifests/PreTestDiscovery")
file(GLOB plans "${plan_dir}/*.json")
if(plans)
    message(FATAL_ERROR
        "PRE_TEST discovery wrote plans during build, before CTest: ${plans}")
endif()

set(test_command "${CTEST_EXECUTABLE}" --test-dir "${TEST_BINARY_DIR}" --output-on-failure)
if(TEST_BUILD_CONFIG)
    list(APPEND test_command --build-config "${TEST_BUILD_CONFIG}")
endif()
execute_process(
    COMMAND ${test_command}
    RESULT_VARIABLE test_result
    OUTPUT_VARIABLE test_stdout
    ERROR_VARIABLE test_stderr)
if(NOT test_result EQUAL 0)
    message(FATAL_ERROR
        "PRE_TEST CTest run failed:\n${test_stdout}${test_stderr}")
endif()
if(NOT test_stdout MATCHES "100% tests passed")
    message(FATAL_ERROR
        "PRE_TEST CTest output did not report a passing scenario:\n${test_stdout}${test_stderr}")
endif()

file(GLOB plans "${plan_dir}/*.json")
list(LENGTH plans plan_count)
if(NOT plan_count EQUAL 2)
    message(FATAL_ERROR
        "Expected PRE_TEST discovery to store exactly two plans, found ${plan_count}: ${plans}")
endif()
