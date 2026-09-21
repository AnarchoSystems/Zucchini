# The manifest is validated against the repository's schema before it is parsed.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/schema.json"
    "${CMAKE_SOURCE_DIR}/stylesheet.schema.json"
)
file(READ "${CMAKE_SOURCE_DIR}/schema.json" ZUCCHINI_SCHEMA_JSON)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/SchemaSource.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/SchemaSource.hpp"
    @ONLY
)
file(READ "${CMAKE_SOURCE_DIR}/stylesheet.schema.json" ZUCCHINI_STYLESHEET_SCHEMA_JSON)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/StylesheetSchemaSource.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/StylesheetSchemaSource.hpp"
    @ONLY
)

add_library(LibZucchini
    src/Diagnostics.cpp
    src/Discovery.cpp
    src/FeatureParser.cpp
    src/MakeZucchini.cpp
    src/ManifestParser.cpp
    src/ManifestStore.cpp
    src/MediaType.cpp
    src/Naming.cpp
    src/SchemaValidator.cpp
    src/Snippets.cpp
    src/SourceLocation.cpp
    src/StepDefManifest.cpp
    src/Stylesheet.cpp
    src/StylesheetParser.cpp
    src/YamlToJson.cpp
    src/Zucchini.cpp
    src/ZucchiniMain.cpp
)
add_library(Zucchini::LibZucchini ALIAS LibZucchini)

target_include_directories(LibZucchini
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(LibZucchini
    PUBLIC
        cucumber::gherkin
        cucumber::messages
        nlohmann_json::nlohmann_json
        GTest::gtest
    PRIVATE
        fkYAML::fkYAML
        nlohmann_json_schema_validator::validator
)

if(ZUCCHINI_BUILD_TESTS)
    add_subdirectory(Test)
endif()
