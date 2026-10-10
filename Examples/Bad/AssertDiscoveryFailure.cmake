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

function(check_discovery review_manifest)
set(update_arguments)
if(EXISTS "${CASE_SOURCE_DIR}/before.yml.bak")
    list(APPEND update_arguments "step_definitions=${review_manifest}")
    file(READ "${CASE_SOURCE_DIR}/before.yml.bak" original HEX)
    file(READ "${review_manifest}" current HEX)
    if(NOT current STREQUAL original)
        message(FATAL_ERROR "Source fixture differs from before.yml.bak; synchronize them before testing")
    endif()
endif()
execute_process(
    COMMAND "${TEST_EXECUTABLE}"
        --gtest_list_tests
        ${update_arguments}
        "feature_dir=features"
        "manifest_dir=${CMAKE_CURRENT_BINARY_DIR}/manifests"
    WORKING_DIRECTORY "${CASE_SOURCE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr)

if(update_arguments)
    file(READ "${review_manifest}" updated HEX)
    # Restore before any assertion can terminate this script.
    configure_file("${CASE_SOURCE_DIR}/before.yml.bak"
        "${review_manifest}" COPYONLY)
    file(READ "${review_manifest}" restored HEX)
    if(NOT restored STREQUAL original)
        message(FATAL_ERROR "Manifest restoration failed")
    endif()
    file(READ "${CASE_SOURCE_DIR}/after.yml" expected HEX)
    if(NOT updated STREQUAL expected)
        message(FATAL_ERROR "Updated manifest differs from after.yml for ${review_manifest}.\n"
            "Process result: ${result}\nstdout: ${actual_stdout}\nstderr: ${actual_stderr}\n"
            "Actual bytes: ${updated}\nExpected bytes: ${expected}")
    endif()
endif()

if(result EQUAL 0)
    message(FATAL_ERROR "Discovery unexpectedly succeeded")
endif()

if(NOT actual_stdout STREQUAL "")
    message(FATAL_ERROR "Discovery wrote unexpected stdout:\n${actual_stdout}")
endif()

file(READ "${EXPECTED_STDERR}" expected_stderr)
if(update_arguments)
    include("${CMAKE_CURRENT_LIST_DIR}/ManifestReviewUrl.cmake")
    manifest_review_url(url "${review_manifest}")
    string(REPLACE "@MANIFEST_URL@" "${url}" expected_stderr "${expected_stderr}")
endif()
normalize_discovery_output(actual_stderr)
normalize_discovery_output(expected_stderr)
if(NOT actual_stderr STREQUAL expected_stderr)
    message(FATAL_ERROR
        "Discovery stderr differs.\n"
        "--- expected ---\n${expected_stderr}"
        "--- actual ---\n${actual_stderr}"
        "--- end ---")
endif()
endfunction()
check_discovery("${CASE_SOURCE_DIR}/features/manifest.yaml")
if(EXISTS "${CASE_SOURCE_DIR}/before.yml.bak")
    check_discovery("${CASE_SOURCE_DIR}/review manifest #%.yml")
endif()
