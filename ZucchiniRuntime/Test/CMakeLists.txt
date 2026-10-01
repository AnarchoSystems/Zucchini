add_executable(ZucchiniRuntimeTest
    DataTableTest.cpp
    DiscoveryTest.cpp
    FeatureParserTest.cpp
    MakeZucchiniTest.cpp
    NamingTest.cpp
    StepContextTest.cpp
    ZucchiniJsonTest.cpp
)

target_include_directories(ZucchiniRuntimeTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(ZucchiniRuntimeTest PRIVATE
    Zucchini::Runtime
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ZucchiniRuntimeTest)