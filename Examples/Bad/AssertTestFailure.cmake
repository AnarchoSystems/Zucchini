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
    RESULT_VARIABLE test_result)

if(test_result EQUAL 0)
    message(FATAL_ERROR "Test unexpectedly succeeded")
endif()