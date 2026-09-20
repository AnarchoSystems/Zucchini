execute_process(
    COMMAND "${ZUCCHINI_EXECUTABLE}"
        -i "features/manifest.yaml"
        -fixture "${FIXTURE}"
        -o "${CMAKE_CURRENT_BINARY_DIR}/generated"
    WORKING_DIRECTORY "${CASE_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr)

if(result EQUAL 0)
    message(FATAL_ERROR "Generator unexpectedly succeeded")
endif()

if(NOT actual_stdout STREQUAL "")
    message(FATAL_ERROR "Generator wrote unexpected stdout:\n${actual_stdout}")
endif()

file(READ "${EXPECTED_STDERR}" expected_stderr)
if(NOT actual_stderr STREQUAL expected_stderr)
    message(FATAL_ERROR
        "Generator stderr differs.\n"
        "--- expected ---\n${expected_stderr}"
        "--- actual ---\n${actual_stderr}"
        "--- end ---")
endif()