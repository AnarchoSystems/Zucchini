set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/schemas/schema.json")
file(READ "${PROJECT_SOURCE_DIR}/schemas/schema.json" ZUCCHINI_SCHEMA_JSON)
configure_file("${PROJECT_SOURCE_DIR}/cmake/SchemaSource.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/SchemaSource.hpp" @ONLY)
add_library(LibZucchini
    src/Diagnostics.cpp
    src/StepDefinitions.cpp
    src/YamlDocument.cpp
    src/ManifestParser.cpp
    src/SchemaValidator.cpp
)
add_library(Zucchini::LibZucchini ALIAS LibZucchini)

target_include_directories(LibZucchini
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_include_directories(LibZucchini PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_link_libraries(LibZucchini PUBLIC
    nlohmann_json::nlohmann_json
    PRIVATE libfyaml::libfyaml nlohmann_json_schema_validator::validator
)

if(ZUCCHINI_BUILD_TESTS)
    add_subdirectory(Test)
endif()
