add_executable(UserTagsTest UserTagsApiTest.cpp)
zucchinify(UserTagsTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE UserTags)
zucchini_add_main(UserTagsTest UserTags)