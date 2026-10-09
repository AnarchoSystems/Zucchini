add_executable(ZucchiniGeneratorTest
    ManifestParserTest.cpp
    StylesheetParserTest.cpp
    LoweringTest.cpp
    ../src/ManifestParser.cpp
    ../src/Lowering.cpp
    ../src/SchemaValidator.cpp
    ../src/Stylesheet.cpp
    ../src/StylesheetParser.cpp
)

target_include_directories(ZucchiniGeneratorTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    "${CMAKE_CURRENT_BINARY_DIR}/../generated"
    "${CMAKE_CURRENT_BINARY_DIR}/../tpp-generated"
)

add_dependencies(ZucchiniGeneratorTest Zucchini)

target_link_libraries(ZucchiniGeneratorTest PRIVATE
    Zucchini::LibZucchini
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator::validator
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ZucchiniGeneratorTest)
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/regression/IRegression.h"
    COMMAND $<TARGET_FILE:Zucchini>
        -i "${CMAKE_CURRENT_SOURCE_DIR}/Regression.yaml"
        -fixture Regression -o "${CMAKE_CURRENT_BINARY_DIR}/regression"
    DEPENDS Zucchini "${CMAKE_CURRENT_SOURCE_DIR}/Regression.yaml"
    VERBATIM
)
add_executable(ZucchiniGeneratedParsingTest
    GeneratedParsingTest.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/regression/IRegression.h"
)
target_include_directories(ZucchiniGeneratedParsingTest PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/regression")
target_link_libraries(ZucchiniGeneratedParsingTest PRIVATE
    Zucchini::Runtime GTest::gtest_main)
gtest_discover_tests(ZucchiniGeneratedParsingTest)
