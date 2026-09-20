function(normalize_discovery_output variable_name)
    set(value "${${variable_name}}")
    string(REPLACE "\r\n" "\n" value "${value}")

    set(has_trailing_newline FALSE)
    if(value MATCHES "\n$")
        set(has_trailing_newline TRUE)
    endif()

    string(REPLACE "\n" ";" lines "${value}")
    set(normalized "")
    foreach(line IN LISTS lines)
        if(line MATCHES "^[A-Za-z]:\\\\"
                OR line MATCHES "^[^ \t][^:]*\\\\.*(:|\\()")
            string(REPLACE "\\" "/" line "${line}")
        endif()

        if(normalized STREQUAL "")
            set(normalized "${line}")
        else()
            string(APPEND normalized "\n${line}")
        endif()
    endforeach()

    if(has_trailing_newline AND NOT normalized STREQUAL "")
        string(APPEND normalized "\n")
    endif()

    set(${variable_name} "${normalized}" PARENT_SCOPE)
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