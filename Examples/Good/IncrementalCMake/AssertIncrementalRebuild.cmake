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

set(generated_dir "${test_binary_dir}/zucchini-generated/Probe")
set(manifest_dir "${test_binary_dir}/zucchini-manifests/Probe")
file(READ "${generated_dir}/generation.marker" generation_marker)
file(READ "${manifest_dir}/discovery.marker" discovery_marker)
if(NOT generation_marker STREQUAL "x" OR NOT discovery_marker STREQUAL "x")
    message(FATAL_ERROR "Initial build must generate and discover exactly once")
endif()

file(GLOB_RECURSE all_object_files
    "${test_binary_dir}/*.o"
    "${test_binary_dir}/*.obj")
set(object_files "")
foreach(object_file IN LISTS all_object_files)
    string(FIND "${object_file}" "Probe.dir/" probe_directory_position)
    if(NOT probe_directory_position EQUAL -1)
        list(APPEND object_files "${object_file}")
    endif()
endforeach()
list(LENGTH object_files object_count)
if(NOT object_count EQUAL 2)
    message(FATAL_ERROR "Expected two Probe object files, found ${object_count}")
endif()
file(GLOB_RECURSE probe_executables
    "${test_binary_dir}/Probe${TEST_EXECUTABLE_SUFFIX}")
list(LENGTH probe_executables executable_count)
if(NOT executable_count EQUAL 1)
    message(FATAL_ERROR
        "Expected one Probe executable, found ${executable_count}: ${probe_executables}")
endif()
list(GET probe_executables 0 probe_executable)
file(TIMESTAMP "${probe_executable}" last_executable_timestamp "%s")
set(unchanged_files ${object_files}
    "${generated_dir}/IProbe.h"
    "${generated_dir}/ProbeTest.cc")
set(first_timestamps "")
foreach(path IN LISTS unchanged_files)
    file(TIMESTAMP "${path}" timestamp "%s")
    list(APPEND first_timestamps "${timestamp}")
endforeach()

execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1.1)
file(APPEND "${test_source_dir}/features/Probe.feature"
    "\n  Scenario: AddedScenario\n    Given a probe\n")
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

file(READ "${generated_dir}/generation.marker" generation_marker)
file(READ "${manifest_dir}/discovery.marker" discovery_marker)
if(NOT generation_marker STREQUAL "x" OR NOT discovery_marker STREQUAL "xx")
    message(FATAL_ERROR "Feature edit must rediscover without running generation")
endif()
set(second_timestamps "")
foreach(path IN LISTS unchanged_files)
    file(TIMESTAMP "${path}" timestamp "%s")
    list(APPEND second_timestamps "${timestamp}")
endforeach()
if(NOT first_timestamps STREQUAL second_timestamps)
    message(FATAL_ERROR "Feature edit regenerated sources or recompiled Probe objects")
endif()
file(TIMESTAMP "${probe_executable}" executable_timestamp "%s")
if(executable_timestamp STREQUAL last_executable_timestamp)
    message(FATAL_ERROR "Feature edit did not relink the Probe executable")
endif()
set(last_executable_timestamp "${executable_timestamp}")
file(GLOB test_scripts "${test_binary_dir}/Probe*_tests.cmake")
set(found_added_scenario FALSE)
foreach(test_script IN LISTS test_scripts)
    file(READ "${test_script}" test_contents)
    string(FIND "${test_contents}" "AddedScenario" scenario_position)
    if(NOT scenario_position EQUAL -1)
        set(found_added_scenario TRUE)
    endif()
endforeach()
if(NOT found_added_scenario)
    message(FATAL_ERROR "Feature edit did not update the discovered scenario list")
endif()

set(expected_marker "xx")
foreach(change IN ITEMS add remove)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1.1)
    if(change STREQUAL "add")
        file(WRITE "${test_source_dir}/features/Additional.feature"
            "Feature: Additional\n\n  Scenario: NewFeatureScenario\n    Given a probe\n")
    else()
        file(REMOVE "${test_source_dir}/features/Additional.feature")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${test_binary_dir}" --target Probe
        RESULT_VARIABLE feature_build_result
        OUTPUT_VARIABLE feature_build_stdout
        ERROR_VARIABLE feature_build_stderr)
    if(NOT feature_build_result EQUAL 0)
        message(FATAL_ERROR "Feature ${change} failed:\n${feature_build_stdout}${feature_build_stderr}")
    endif()
    string(APPEND expected_marker "x")
    file(READ "${test_binary_dir}/build.marker" build_marker)
    file(READ "${generated_dir}/generation.marker" generation_marker)
    file(READ "${manifest_dir}/discovery.marker" discovery_marker)
    if(NOT build_marker STREQUAL expected_marker OR NOT generation_marker STREQUAL "x"
            OR NOT discovery_marker STREQUAL expected_marker)
        message(FATAL_ERROR "Feature ${change} must relink and rediscover without generation")
    endif()
    set(timestamps "")
    foreach(path IN LISTS unchanged_files)
        file(TIMESTAMP "${path}" timestamp "%s")
        list(APPEND timestamps "${timestamp}")
    endforeach()
    if(NOT first_timestamps STREQUAL timestamps)
        message(FATAL_ERROR "Feature ${change} regenerated sources or recompiled Probe objects")
    endif()
    file(TIMESTAMP "${probe_executable}" executable_timestamp "%s")
    if(executable_timestamp STREQUAL last_executable_timestamp)
        message(FATAL_ERROR "Feature ${change} did not relink the Probe executable")
    endif()
    set(last_executable_timestamp "${executable_timestamp}")
    file(GLOB test_scripts "${test_binary_dir}/Probe*_tests.cmake")
    set(found_new_feature FALSE)
    foreach(test_script IN LISTS test_scripts)
        file(READ "${test_script}" test_contents)
        string(FIND "${test_contents}" "NewFeatureScenario" scenario_position)
        if(NOT scenario_position EQUAL -1)
            set(found_new_feature TRUE)
        endif()
    endforeach()
    if((change STREQUAL "add" AND NOT found_new_feature)
            OR (change STREQUAL "remove" AND found_new_feature))
        message(FATAL_ERROR "Feature ${change} did not update the discovered scenario list")
    endif()
endforeach()

execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1.1)
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${test_binary_dir}" --target Probe
    RESULT_VARIABLE no_op_result
    OUTPUT_VARIABLE no_op_stdout
    ERROR_VARIABLE no_op_stderr)
if(NOT no_op_result EQUAL 0)
    message(FATAL_ERROR "No-op rebuild failed:\n${no_op_stdout}${no_op_stderr}")
endif()
file(READ "${test_binary_dir}/build.marker" no_op_marker)
file(READ "${generated_dir}/generation.marker" generation_marker)
file(READ "${manifest_dir}/discovery.marker" discovery_marker)
set(no_op_expected_marker "${expected_marker}")
if(TEST_GENERATOR MATCHES "Visual Studio")
    # Visual Studio runs target POST_BUILD commands whenever its project is built,
    # even if the linker has no work to do.
    string(APPEND no_op_expected_marker "x")
endif()
if(NOT no_op_marker STREQUAL no_op_expected_marker OR NOT generation_marker STREQUAL "x"
        OR NOT discovery_marker STREQUAL no_op_expected_marker)
    message(FATAL_ERROR
        "No-op rebuild unexpectedly relinked, regenerated, or rediscovered: "
        "build marker '${no_op_marker}' (expected '${no_op_expected_marker}'), "
        "generation marker '${generation_marker}' (expected 'x'), "
        "discovery marker '${discovery_marker}' (expected '${no_op_expected_marker}')\n"
        "${no_op_stdout}${no_op_stderr}")
endif()
file(TIMESTAMP "${probe_executable}" executable_timestamp "%s")
if(NOT executable_timestamp STREQUAL last_executable_timestamp)
    message(FATAL_ERROR
        "No-op rebuild relinked the Probe executable: timestamp changed from "
        "'${last_executable_timestamp}' to '${executable_timestamp}'\n"
        "${no_op_stdout}${no_op_stderr}")
endif()
set(expected_marker "${no_op_expected_marker}")

execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1.1)
file(APPEND "${test_source_dir}/features/Probe.yaml" "\n# trigger generation\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${test_binary_dir}" --target Probe
    RESULT_VARIABLE manifest_build_result
    OUTPUT_VARIABLE manifest_build_stdout
    ERROR_VARIABLE manifest_build_stderr)
if(NOT manifest_build_result EQUAL 0)
    message(FATAL_ERROR "Manifest rebuild failed:\n${manifest_build_stdout}${manifest_build_stderr}")
endif()
file(TIMESTAMP "${probe_executable}" executable_timestamp "%s")
if(executable_timestamp STREQUAL last_executable_timestamp)
    message(FATAL_ERROR "Manifest edit did not relink the Probe executable")
endif()
file(READ "${generated_dir}/generation.marker" generation_marker)
file(READ "${manifest_dir}/discovery.marker" discovery_marker)
file(READ "${test_binary_dir}/build.marker" manifest_marker)
string(APPEND expected_marker "x")
if(NOT generation_marker STREQUAL "xx" OR NOT discovery_marker STREQUAL expected_marker
        OR NOT manifest_marker STREQUAL expected_marker)
    message(FATAL_ERROR "Manifest edit must regenerate, relink, and rediscover")
endif()