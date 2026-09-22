add_executable(LegacyStringTest)
zucchinify(LegacyStringTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE LegacyStringDemo
    STYLESHEET "${CMAKE_CURRENT_SOURCE_DIR}/features/LegacyString.stylesheet.yaml"
)
zucchini_add_main(LegacyStringTest LegacyStringDemo)
