set(manifest "${CASE_SOURCE_DIR}/features/manifest.yaml")
file(READ "${manifest}" original)
execute_process(
    COMMAND "${CTEST_EXECUTABLE}" --test-dir "${CASE_BINARY_DIR}" -C "${TEST_CONFIG}" -N
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
file(READ "${manifest}" updated)
# Restore before asserting so a failed regression does not leave source edits.
file(WRITE "${manifest}" "${original}")
if(NOT updated STREQUAL original)
    message(FATAL_ERROR "PRINT discovery modified the source manifest")
endif()
if(result EQUAL 0)
    message(FATAL_ERROR "Discovery unexpectedly succeeded")
endif()
if(NOT stderr MATCHES "add these step definitions:" OR
   NOT stderr MATCHES "methodName: i_have_apples" OR
   NOT stderr MATCHES "type: int")
    message(FATAL_ERROR "Missing snippets on stderr:\n${stderr}\n${stdout}")
endif()
