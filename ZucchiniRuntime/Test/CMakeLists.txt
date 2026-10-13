add_executable(ZucchiniRuntimeTest
    RuleReportTest.cpp
    RuleTest.cpp
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
# The viewer needs only a browser. Node adds model/rendering validation when available.
find_program(ZUCCHINI_NODE_EXECUTABLE NAMES node nodejs)
if(ZUCCHINI_NODE_EXECUTABLE)
    add_test(NAME RuleDashboard.StatusAndRendering
        COMMAND "${ZUCCHINI_NODE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/RuleDashboardTest.js"
            "${CMAKE_CURRENT_SOURCE_DIR}/../src/RuleDashboard.inc")
endif()
