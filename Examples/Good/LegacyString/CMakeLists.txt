add_executable(LegacyStringTest)
zucchinify(LegacyStringTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE LegacyStringDemo
)
