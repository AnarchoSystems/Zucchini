add_executable(AssertionJustificationTest)
zucchinify(AssertionJustificationTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Basket
    RULE_COVERAGE_REPORT
    UNJUSTIFIED_ASSERTION_COVERAGE
)
zucchini_add_main(AssertionJustificationTest Basket)

add_test(NAME Rules.AssertionJustification
    COMMAND "${CMAKE_COMMAND}"
        "-DEXECUTABLE=$<TARGET_FILE:AssertionJustificationTest>"
        "-DFEATURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/features"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/justification-test/$<CONFIG>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/AssertJustification.cmake")
