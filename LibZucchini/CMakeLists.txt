add_library(LibZucchini
    src/Diagnostics.cpp
    src/MakeZucchini.cpp
    src/ManifestParser.cpp
    src/StepDefManifest.cpp
    src/Zucchini.cpp
)
add_library(Zucchini::LibZucchini ALIAS LibZucchini)

target_include_directories(LibZucchini
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(LibZucchini
    PUBLIC
        cucumber::gherkin
        cucumber::messages
        nlohmann_json::nlohmann_json
    PRIVATE
        fkYAML::fkYAML
)

if(ZUCCHINI_BUILD_TESTS)
    add_subdirectory(Test)
endif()
