add_executable(ZucchiniGeneratorTest
    ManifestParserTest.cpp
    StylesheetParserTest.cpp
    LoweringTest.cpp
    ../src/ManifestParser.cpp
    ../src/Lowering.cpp
    ../src/SchemaValidator.cpp
    ../src/Stylesheet.cpp
    ../src/StylesheetParser.cpp
    ../src/YamlToJson.cpp
)

target_include_directories(ZucchiniGeneratorTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    "${CMAKE_CURRENT_BINARY_DIR}/../generated"
    "${CMAKE_CURRENT_BINARY_DIR}/../tpp-generated"
)

add_dependencies(ZucchiniGeneratorTest ZucchiniTppTools)

target_link_libraries(ZucchiniGeneratorTest PRIVATE
    Zucchini::LibZucchini
    fkYAML::fkYAML
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator::validator
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ZucchiniGeneratorTest)