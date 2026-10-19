# The versioned schema owns the wire-format version emitted by the runtime.
set(ZUCCHINI_RULE_REPORT_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/../schemas/rules-report.v2.2.0.schema.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ZUCCHINI_RULE_REPORT_SCHEMA}")
file(READ "${ZUCCHINI_RULE_REPORT_SCHEMA}" ZUCCHINI_RULE_REPORT_SCHEMA_JSON)
string(JSON ZUCCHINI_RULE_REPORT_VERSION GET "${ZUCCHINI_RULE_REPORT_SCHEMA_JSON}" properties schema_version const)
string(JSON ZUCCHINI_RULE_REPORT_SCHEMA_URI GET "${ZUCCHINI_RULE_REPORT_SCHEMA_JSON}" "$id")
configure_file(src/RuleReportVersion.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/RuleReportVersion.hpp" @ONLY)

add_library(ZucchiniRuntime
    src/AssertedFact.cpp
    src/AssertionJustification.cpp
    src/RuleReport.cpp
    src/ExecutionPlan.cpp
    src/FeatureParser.cpp
    src/MakeZucchini.cpp
    src/ManifestStore.cpp
    src/ManifestUpdate.cpp
    src/MediaType.cpp
    src/Naming.cpp
    src/PlanDiscovery.cpp
    src/Snippets.cpp
    src/SourceLocation.cpp
    src/Zucchini.cpp
)
add_library(Zucchini::Runtime ALIAS ZucchiniRuntime)

target_include_directories(ZucchiniRuntime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(ZucchiniRuntime
    PUBLIC
        Zucchini::LibZucchini
        cucumber::messages
        nlohmann_json::nlohmann_json
    PRIVATE
        cucumber::gherkin
        libfyaml::libfyaml
    )

if(ZUCCHINI_BUILD_TESTS)
    add_subdirectory(Test)
endif()