add_executable(TablesTest TablesJsonTest.cpp)
zucchinify(TablesTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Tables
)
zucchini_add_main(TablesTest Tables)
