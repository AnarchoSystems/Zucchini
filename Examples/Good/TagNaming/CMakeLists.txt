add_executable(TagNamingTest)
zucchinify(TagNamingTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE TagNaming
    STYLESHEET "${CMAKE_CURRENT_SOURCE_DIR}/features/TagNaming.stylesheet.yaml")
zucchini_add_main(TagNamingTest TagNaming)