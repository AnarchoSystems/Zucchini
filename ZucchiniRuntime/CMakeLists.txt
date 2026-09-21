add_library(ZucchiniRuntime
    src/Discovery.cpp
    src/FeatureParser.cpp
    src/MakeZucchini.cpp
    src/ManifestStore.cpp
    src/MediaType.cpp
    src/Naming.cpp
    src/Snippets.cpp
    src/SourceLocation.cpp
    src/YamlToJson.cpp
    src/Zucchini.cpp
    src/ZucchiniMain.cpp
)
add_library(Zucchini::Runtime ALIAS ZucchiniRuntime)

target_include_directories(ZucchiniRuntime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(ZucchiniRuntime
    PUBLIC
        Zucchini::LibZucchini
        cucumber::messages
        GTest::gtest
        nlohmann_json::nlohmann_json
    PRIVATE
        cucumber::gherkin
        fkYAML::fkYAML
)

if(ZUCCHINI_BUILD_TESTS)
    add_subdirectory(Test)
endif()