add_executable(NotesTest)
zucchinify(NotesTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    RULE_COVERAGE_REPORT
    RULE_COVERAGE_REPORT_PATH "${CMAKE_CURRENT_BINARY_DIR}/reports/$<CONFIG>/rule coverage"
    FIXTURE Notes
)
zucchini_add_main(NotesTest Notes)

add_test(NAME Rules.DiscoveryReport
    COMMAND "${CMAKE_COMMAND}"
        "-DEXECUTABLE=$<TARGET_FILE:NotesTest>"
        "-DFEATURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/features"
        "-DCONFIGURED_REPORT_DIR=${CMAKE_CURRENT_BINARY_DIR}/reports/$<CONFIG>/rule coverage"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/report-test/$<CONFIG>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/AssertRuleReport.cmake")
