set(manifest_dir "${CMAKE_CURRENT_BINARY_DIR}/manifests")

execute_process(
    COMMAND "${TEST_EXECUTABLE}"
        --gtest_list_tests
        "feature_dir=features"
        "manifest_dir=${manifest_dir}"
    WORKING_DIRECTORY "${CASE_SOURCE_DIR}"
    RESULT_VARIABLE discovery_result
    OUTPUT_VARIABLE discovery_stdout
    ERROR_VARIABLE discovery_stderr)

if(NOT discovery_result EQUAL 0)
    message(FATAL_ERROR
        "Test failed before its declared stage (discovery):\n"
        "${discovery_stdout}${discovery_stderr}")
endif()

execute_process(
    COMMAND "${TEST_EXECUTABLE}"
        --gtest_color=no
        --gtest_brief=1
        "--gtest_filter=Zucchinis/*"
        "manifest_dir=${manifest_dir}"
    WORKING_DIRECTORY "${CASE_SOURCE_DIR}"
    RESULT_VARIABLE test_result
    OUTPUT_VARIABLE actual_output
    ERROR_VARIABLE actual_stderr)

if(test_result EQUAL 0)
    message(FATAL_ERROR "Test unexpectedly succeeded")
endif()

if(NOT actual_stderr STREQUAL "")
    message(FATAL_ERROR "Test wrote unexpected stderr:\n${actual_stderr}")
endif()

get_filename_component(source_root "${CASE_SOURCE_DIR}/../../.." ABSOLUTE)
string(REPLACE "${source_root}/" "" actual_output "${actual_output}")
string(REGEX REPLACE "[0-9]+ ms" "TIME" actual_output "${actual_output}")
file(READ "${EXPECTED_OUTPUT}" expected_output)
string(REPLACE "\r\n" "\n" actual_output "${actual_output}")
string(REPLACE "\r\n" "\n" expected_output "${expected_output}")
if(NOT actual_output STREQUAL expected_output)
    message(FATAL_ERROR
        "Test output differs.\n"
        "--- expected ---\n${expected_output}"
        "--- actual ---\n${actual_output}"
        "--- end ---")
endif()
