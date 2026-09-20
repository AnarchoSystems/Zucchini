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