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
    COMMAND "${CMAKE_COMMAND}" --build "${test_binary_dir}" --target Probe
    RESULT_VARIABLE first_build_result
    OUTPUT_VARIABLE first_build_stdout
    ERROR_VARIABLE first_build_stderr)
if(NOT first_build_result EQUAL 0)
    message(FATAL_ERROR "Initial incremental example build failed:\n${first_build_stdout}${first_build_stderr}")
endif()

file(GLOB discovery_scripts "${test_binary_dir}/Probe*_discovery.cmake")
list(LENGTH discovery_scripts discovery_script_count)
set(discovery_contents "")
if(discovery_script_count GREATER 0)
    # CMake may emit one script per configuration on multi-config generators.
    list(GET discovery_scripts 0 discovery_script)
    file(READ "${discovery_script}" discovery_contents)
else()
    # Newer CMake builds can emit only include/tests files and keep gtest discovery
    # arguments in generated build metadata rather than Probe*_discovery.cmake scripts.
    file(GLOB_RECURSE probe_build_metadata
        "${test_binary_dir}/CMakeFiles/*.make"
        "${test_binary_dir}/CMakeFiles/*.ninja"
        "${test_binary_dir}/CMakeFiles/*.txt"
        "${test_binary_dir}/*.make"
        "${test_binary_dir}/*.ninja"
        "${test_binary_dir}/*.vcxproj")
    foreach(metadata_file IN LISTS probe_build_metadata)
        file(READ "${metadata_file}" metadata_contents)
        string(FIND "${metadata_contents}" "GoogleTestAddTests.cmake" has_gtest_add_tests)
        string(FIND "${metadata_contents}" "TEST_EXTRA_ARGS=" has_test_extra_args)
        if(NOT has_gtest_add_tests EQUAL -1 AND NOT has_test_extra_args EQUAL -1)
            set(discovery_contents "${metadata_contents}")
            break()
        endif()
    endforeach()
endif()

if(discovery_contents STREQUAL "")
    message(FATAL_ERROR
        "Could not locate Probe gtest discovery metadata in ${test_binary_dir}")
endif()

foreach(expected IN ITEMS
        "Zucchini."
        "23"
        "manifest_dir="
        "runtime_probe=1"
        "feature_dir="
        "discovery_probe=1")
    string(FIND "${discovery_contents}" "${expected}" expected_position)
    if(expected_position EQUAL -1)
        message(FATAL_ERROR
            "Probe discovery metadata does not contain '${expected}':\n${discovery_contents}")
    endif()
endforeach()

string(FIND "${discovery_contents}" "TIMEOUT;17" timeout_legacy_position)
if(timeout_legacy_position EQUAL -1)
    string(REGEX MATCH "TEST_PROPERTIES[^\\n\\r]*TIMEOUT[^\\n\\r]*17" timeout_verbose_match
        "${discovery_contents}")
    if(timeout_verbose_match STREQUAL "")
        message(FATAL_ERROR
            "Probe discovery metadata does not contain timeout property 'TIMEOUT=17':\n${discovery_contents}")
    endif()
endif()

string(REGEX MATCH "TEST_EXTRA_ARGS[^\\n\\r]*" test_extra_args "${discovery_contents}")
if(test_extra_args STREQUAL "")
    string(REGEX MATCH "TEST_EXTRA_ARGS=([^\"\\n\\r]*|\"[^\"]*\")" test_extra_args "${discovery_contents}")
endif()
if(test_extra_args STREQUAL "")
    message(FATAL_ERROR
        "Could not locate TEST_EXTRA_ARGS in Probe discovery metadata:\n${discovery_contents}")
endif()
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