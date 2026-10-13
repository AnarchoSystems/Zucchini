add_executable(BusinessRulesTest)
zucchinify(BusinessRulesTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Shop
    RULE_COVERAGE_REPORT
)
zucchini_add_main(BusinessRulesTest Shop)

add_test(NAME Rules.BusinessInvariantDemo
    COMMAND "${CMAKE_COMMAND}"
        "-DEXECUTABLE=$<TARGET_FILE:BusinessRulesTest>"
        "-DFEATURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/features"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/report-test/$<CONFIG>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/AssertBusinessRules.cmake")
