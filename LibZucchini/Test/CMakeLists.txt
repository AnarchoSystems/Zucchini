add_executable(LibZucchiniTest
    DiscoveryTest.cpp
    FeatureParserTest.cpp
    MakeZucchiniTest.cpp
    ManifestParserTest.cpp
    NamingTest.cpp
    ZucchiniJsonTest.cpp
)

target_link_libraries(LibZucchiniTest
    PRIVATE
        Zucchini::LibZucchini
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(LibZucchiniTest)
