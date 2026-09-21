add_executable(LibZucchiniTest DiagnosticsTest.cpp)

target_link_libraries(LibZucchiniTest
    PRIVATE
        Zucchini::LibZucchini
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(LibZucchiniTest)
