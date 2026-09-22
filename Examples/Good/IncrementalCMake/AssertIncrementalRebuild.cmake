set(test_source_dir "${TEST_ROOT}/source")
set(test_binary_dir "${TEST_ROOT}/build")
file(REMOVE_RECURSE "${TEST_ROOT}")
file(COPY "${TEMPLATE_SOURCE_DIR}/" DESTINATION "${test_source_dir}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${test_source_dir}"
        -B "${test_binary_dir}"
        -D "ZUCCHINIFY_MODULE_DIR=${ZUCCHINIFY_MODULE_DIR}"
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Incremental example failed to configure:\n${configure_stdout}${configure_stderr}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${test_binary_dir}" --target Probe --verbose
    RESULT_VARIABLE first_build_result
    OUTPUT_VARIABLE first_build_stdout
    ERROR_VARIABLE first_build_stderr)
if(NOT first_build_result EQUAL 0)
    message(FATAL_ERROR "Initial incremental example build failed:\n${first_build_stdout}${first_build_stderr}")
endif()

file(GLOB discovery_scripts "${test_binary_dir}/Probe*_discovery.cmake")
list(LENGTH discovery_scripts discovery_script_count)
if(discovery_script_count EQUAL 1)
    list(GET discovery_scripts 0 discovery_details)
    file(READ "${discovery_details}" discovery_contents)
    set(expected_fragments
        "TEST_PREFIX [==[Zucchini.]==]"
        "TEST_DISCOVERY_TIMEOUT [==[23]==]"
        "TEST_PROPERTIES [==[TIMEOUT]==] [==[17]==]"
        "TEST_EXTRA_ARGS [==[manifest_dir="
        "[==[runtime_probe=1]==]"
        "TEST_DISCOVERY_EXTRA_ARGS [==[feature_dir="
        "[==[discovery_probe=1]==]")
    set(test_extra_args_pattern "TEST_EXTRA_ARGS[^\\n\\r]*")
elseif(discovery_script_count EQUAL 0)
    set(discovery_details "the verbose Probe build output")
    set(discovery_contents "${first_build_stdout}${first_build_stderr}")
    set(expected_fragments
        "TEST_EXTRA_ARGS=manifest_dir="
        "runtime_probe=1"
        "TEST_DISCOVERY_EXTRA_ARGS=feature_dir="
        "discovery_probe=1"
        "TEST_DISCOVERY_TIMEOUT=23"
        "TEST_PROPERTIES=TIMEOUT;17"
        "TEST_PREFIX=Zucchini.")
    set(test_extra_args_pattern "TEST_EXTRA_ARGS=[^\\n\\r]*")
else()
    message(FATAL_ERROR
        "Expected at most one Probe discovery script, found ${discovery_script_count}: ${discovery_scripts}")
endif()

foreach(expected IN LISTS expected_fragments)
    string(FIND "${discovery_contents}" "${expected}" expected_position)
    if(expected_position EQUAL -1)
        message(FATAL_ERROR
            "Probe discovery details in ${discovery_details} do not contain '${expected}':\n${discovery_contents}")
    endif()
endforeach()

string(REGEX MATCH "${test_extra_args_pattern}" test_extra_args "${discovery_contents}")
foreach(option IN ITEMS TEST_PREFIX PROPERTIES DISCOVERY_TIMEOUT)
    string(FIND "${test_extra_args}" "${option}" option_position)
    if(NOT option_position EQUAL -1)
        message(FATAL_ERROR
            "gtest_discover_tests option ${option} leaked into ${test_extra_args}")
    endif()
endforeach()

file(READ "${test_binary_dir}/build.marker" first_marker)
if(NOT first_marker STREQUAL "x")
    message(FATAL_ERROR "Initial build marker should be 'x', got '${first_marker}'")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1.1)
file(APPEND "${test_source_dir}/features/Probe.feature" "\n# trigger incremental discovery\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${test_binary_dir}" --target Probe
    RESULT_VARIABLE second_build_result
    OUTPUT_VARIABLE second_build_stdout
    ERROR_VARIABLE second_build_stderr)
if(NOT second_build_result EQUAL 0)
    message(FATAL_ERROR "Incremental rebuild failed:\n${second_build_stdout}${second_build_stderr}")
endif()

file(READ "${test_binary_dir}/build.marker" second_marker)
if(NOT second_marker STREQUAL "xx")
    message(FATAL_ERROR
        "Feature edit did not relink the test target; expected marker 'xx', got '${second_marker}'")
endif()