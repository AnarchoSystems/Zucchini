add_executable(NotesTest)
zucchinify(NotesTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Notes
)
zucchini_add_main(NotesTest Notes)
