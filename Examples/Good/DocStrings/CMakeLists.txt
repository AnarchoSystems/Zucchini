add_executable(DocStringsTest)
zucchinify(DocStringsTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE DocStrings
)
zucchini_add_main(DocStringsTest DocStrings)
