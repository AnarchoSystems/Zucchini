# The manifests and stylesheets are generator input, never runtime API.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/schemas/schema.json"
    "${CMAKE_SOURCE_DIR}/schemas/stylesheet.schema.json"
)
file(READ "${CMAKE_SOURCE_DIR}/schemas/schema.json" ZUCCHINI_SCHEMA_JSON)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/SchemaSource.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/SchemaSource.hpp"
    @ONLY
)
file(READ "${CMAKE_SOURCE_DIR}/schemas/stylesheet.schema.json" ZUCCHINI_STYLESHEET_SCHEMA_JSON)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/StylesheetSchemaSource.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/StylesheetSchemaSource.hpp"
    @ONLY
)

set(ZUCCHINI_GENERATOR_SOURCES
    src/Lowering.cpp
    src/ManifestParser.cpp
    src/SchemaValidator.cpp
    src/Stylesheet.cpp
    src/StylesheetParser.cpp
    src/YamlToJson.cpp
)

add_executable(Zucchini
    src/main.cpp
    ${ZUCCHINI_GENERATOR_SOURCES}
)

zucchini_add_tpp(Zucchini
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tpp"
    NAME ZucchiniTemplates
    NAMESPACE nZucchiniTemplates
)

target_link_libraries(Zucchini PRIVATE
    Zucchini::LibZucchini
    fkYAML::fkYAML
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator::validator
)

target_include_directories(Zucchini PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

if(ZUCCHINI_BUILD_TESTS)
    add_subdirectory(Test)
endif()
