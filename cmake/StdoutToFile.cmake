if(NOT DEFINED CMD OR NOT DEFINED OUT)
    message(FATAL_ERROR "StdoutToFile.cmake requires CMD and OUT")
endif()

execute_process(
    COMMAND "${CMD}" ${ARGS}
    OUTPUT_FILE "${OUT}"
    RESULT_VARIABLE result
    ERROR_VARIABLE error_output
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "${CMD} failed with exit code ${result}: ${error_output}")
endif()
