add_executable(ScalarListsTest ScalarLists.h)
zucchinify(ScalarListsTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE ScalarLists)
zucchini_add_main(ScalarListsTest ScalarLists)
