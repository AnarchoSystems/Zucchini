add_executable(CalculatorTest)
zucchinify(CalculatorTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Calculator
)
zucchini_add_main(CalculatorTest Calculator)
