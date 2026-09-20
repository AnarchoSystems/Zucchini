function(normalize_discovery_output variable_name)
    set(value "${${variable_name}}")
    string(REPLACE "\r\n" "\n" value "${value}")

    set(previous_value "")
    while(NOT value STREQUAL previous_value)
        set(previous_value "${value}")
        string(REGEX REPLACE "([^ \n]+)\\\\([^ \n]+\\.(feature|ya?ml))" "\\1/\\2" value "${value}")
    endwhile()

    set(${variable_name} "${value}" PARENT_SCOPE)
endfunction()

execute_process(
    COMMAND "${TEST_EXECUTABLE}"
        --gtest_list_tests
        "feature_dir=features"
        "manifest_dir=${CMAKE_CURRENT_BINARY_DIR}/manifests"
    WORKING_DIRECTORY "${CASE_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr)

if(result EQUAL 0)
    message(FATAL_ERROR "Discovery unexpectedly succeeded")
endif()

if(NOT actual_stdout STREQUAL "")
    message(FATAL_ERROR "Discovery wrote unexpected stdout:\n${actual_stdout}")
endif()

file(READ "${EXPECTED_STDERR}" expected_stderr)
normalize_discovery_output(actual_stderr)
normalize_discovery_output(expected_stderr)
if(NOT actual_stderr STREQUAL expected_stderr)
    message(FATAL_ERROR
        "Discovery stderr differs.\n"
        "--- expected ---\n${expected_stderr}"
        "--- actual ---\n${actual_stderr}"
        "--- end ---")
endif()