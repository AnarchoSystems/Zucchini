include_guard(GLOBAL)

include(GoogleTest)

function(zucchini_add_main target fixture)
    set(main_source "${CMAKE_CURRENT_BINARY_DIR}/${fixture}Main.cc")
    file(GENERATE OUTPUT "${main_source}" CONTENT
        "#include <gtest/gtest.h>\n#include \"${fixture}.h\"\nint main(int argc, char** argv) { return n${fixture}::ZucchiniMain(argc, argv); }\n")
    target_sources(${target} PRIVATE "${main_source}")
endfunction()

# Generates the fixture sources for <FIXTURE> from the single YAML manifest in <FEATURE_DIR>,
# attaches them to <target> and wires gtest discovery to the feature files.
function(zucchinify target)
    # Discovery arguments rely on gtest_discover_tests(DISCOVERY_EXTRA_ARGS).
    if(CMAKE_VERSION VERSION_LESS 3.31)
        message(FATAL_ERROR "zucchinify(${target}) requires CMake 3.31 or newer; this is CMake ${CMAKE_VERSION}")
    endif()
    cmake_parse_arguments(ZUCCHINIFY "RULE_COVERAGE_REPORT;UNJUSTIFIED_ASSERTION_COVERAGE" "FEATURE_DIR;FIXTURE;DISCOVERY_MODE;SNIPPET_MODE;STYLESHEET;RULE_COVERAGE_REPORT_PATH;UNJUSTIFIED_ASSERTION_SEVERITY;MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO" "EXTRA_ARGS;DISCOVERY_EXTRA_ARGS" ${ARGN})

    if(NOT ZUCCHINIFY_FEATURE_DIR OR NOT ZUCCHINIFY_FIXTURE)
        message(FATAL_ERROR "zucchinify(${target}) requires FEATURE_DIR and FIXTURE")
    endif()
    if(NOT ZUCCHINIFY_DISCOVERY_MODE)
        set(ZUCCHINIFY_DISCOVERY_MODE POST_BUILD)
    endif()
    if(NOT DEFINED ZUCCHINIFY_SNIPPET_MODE)
        set(ZUCCHINIFY_SNIPPET_MODE MERGE)
    endif()
    if("SNIPPET_MODE" IN_LIST ZUCCHINIFY_KEYWORDS_MISSING_VALUES OR
       NOT ZUCCHINIFY_SNIPPET_MODE MATCHES "^(MERGE|PRINT)$")
        message(FATAL_ERROR "zucchinify(${target}): SNIPPET_MODE must be MERGE or PRINT")
    endif()

    # Choosing a severity or limit implies the check, like RULE_COVERAGE_REPORT_PATH.
    if(ZUCCHINIFY_UNJUSTIFIED_ASSERTION_SEVERITY OR
       DEFINED ZUCCHINIFY_MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO)
        set(ZUCCHINIFY_UNJUSTIFIED_ASSERTION_COVERAGE TRUE)
    endif()
    if(NOT ZUCCHINIFY_UNJUSTIFIED_ASSERTION_SEVERITY)
        set(ZUCCHINIFY_UNJUSTIFIED_ASSERTION_SEVERITY WARNING)
    endif()
    string(TOLOWER "${ZUCCHINIFY_UNJUSTIFIED_ASSERTION_SEVERITY}" unjustified_assertion_severity)
    if(NOT unjustified_assertion_severity MATCHES "^(error|warning|info)$")
        message(FATAL_ERROR
            "zucchinify(${target}): UNJUSTIFIED_ASSERTION_SEVERITY must be ERROR, WARNING or INFO")
    endif()
    if(NOT DEFINED ZUCCHINIFY_MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO)
        set(ZUCCHINIFY_MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO 2)
    endif()
    if(NOT ZUCCHINIFY_MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO MATCHES "^[0-9]+$")
        message(FATAL_ERROR
            "zucchinify(${target}): MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO must be a nonnegative integer")
    endif()

    file(GLOB manifests CONFIGURE_DEPENDS
        "${ZUCCHINIFY_FEATURE_DIR}/*.yaml"
        "${ZUCCHINIFY_FEATURE_DIR}/*.yml")
    if(ZUCCHINIFY_STYLESHEET)
        # A stylesheet living next to the manifest still matches the *.yaml glob; it is supplied
        # explicitly, so it does not count as (and must not be mistaken for) the fixture's manifest.
        list(REMOVE_ITEM manifests "${ZUCCHINIFY_STYLESHEET}")
    endif()
    list(LENGTH manifests manifest_count)
    if(NOT manifest_count EQUAL 1)
        message(FATAL_ERROR
            "zucchinify(${target}): expected exactly one .yaml/.yml in ${ZUCCHINIFY_FEATURE_DIR}, found ${manifest_count}")
    endif()
    list(GET manifests 0 manifest)
    set(snippet_args)
    if(ZUCCHINIFY_SNIPPET_MODE STREQUAL "MERGE")
        list(APPEND snippet_args "step_definitions=${manifest}")
    endif()

    set(generated "${CMAKE_CURRENT_BINARY_DIR}/zucchini-generated/${ZUCCHINIFY_FIXTURE}")
    set(manifest_dir "${CMAKE_CURRENT_BINARY_DIR}/zucchini-manifests/${ZUCCHINIFY_FIXTURE}")
    set(header "${generated}/I${ZUCCHINIFY_FIXTURE}.h")
    set(test_source "${generated}/${ZUCCHINIFY_FIXTURE}Test.cc")
    file(GLOB_RECURSE features CONFIGURE_DEPENDS "${ZUCCHINIFY_FEATURE_DIR}/*.feature")
    file(GLOB zucchini_templates CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/../Zucchini/tpp/*.tpp"
        "${CMAKE_CURRENT_LIST_DIR}/../Zucchini/tpp/tpp-config.json")

    set(style_args "")
    if(ZUCCHINIFY_STYLESHEET)
        set(style_args -style "${ZUCCHINIFY_STYLESHEET}")
    endif()

    add_custom_command(
        OUTPUT "${header}" "${test_source}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${generated}"
        COMMAND $<TARGET_FILE:Zucchini> -i "${manifest}" -fixture "${ZUCCHINIFY_FIXTURE}" -o "${generated}" ${style_args}
        DEPENDS Zucchini "${manifest}" ${ZUCCHINIFY_STYLESHEET} ${zucchini_templates}
        COMMENT "Zucchini: generating ${ZUCCHINIFY_FIXTURE} fixture"
        VERBATIM
    )

    target_sources(${target} PRIVATE "${test_source}" "${header}")
    target_include_directories(${target} PRIVATE "${generated}" "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${target} PRIVATE Zucchini::Runtime GTest::gtest)

    # Features are discovery inputs, not generation or compilation inputs.
    # Track the file list too, so removing a feature also triggers discovery.
    set(feature_list "${manifest_dir}/features.txt")
    file(GENERATE OUTPUT "${feature_list}" CONTENT "${features}\n")
    set(feature_stamp "${CMAKE_CURRENT_BINARY_DIR}/zucchini-manifests/${ZUCCHINIFY_FIXTURE}/features.stamp")
    add_custom_command(
        OUTPUT "${feature_stamp}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/zucchini-manifests/${ZUCCHINIFY_FIXTURE}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${feature_stamp}"
        DEPENDS "${feature_list}" ${features}
        VERBATIM
    )
    add_custom_target(${target}-feature-stamp DEPENDS "${feature_stamp}")
    add_dependencies(${target} ${target}-feature-stamp)
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${feature_stamp}")
    if(NOT CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
        set(feature_link_target "${target}-feature-link")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${features})
        set(feature_signature "")
        foreach(feature IN LISTS features)
            file(SHA256 "${feature}" feature_hash)
            string(APPEND feature_signature "${feature}:${feature_hash}\n")
        endforeach()
        string(SHA256 feature_signature_hash "${feature_signature}")
        set(feature_link_source "${manifest_dir}/FeatureLinkDependency.cpp")
        file(GENERATE OUTPUT "${feature_link_source}"
            CONTENT "static constexpr char zucchini_feature_signature[] = \"${feature_signature_hash}\";\n")
        add_library(${feature_link_target} OBJECT "${feature_link_source}")
        target_sources(${target} PRIVATE "$<TARGET_OBJECTS:${feature_link_target}>")
    endif()

    set(rule_coverage_args)
    if(ZUCCHINIFY_RULE_COVERAGE_REPORT OR ZUCCHINIFY_RULE_COVERAGE_REPORT_PATH)
        if(ZUCCHINIFY_RULE_COVERAGE_REPORT_PATH)
            get_filename_component(rule_report_path "${ZUCCHINIFY_RULE_COVERAGE_REPORT_PATH}"
                ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
        else()
            set(rule_report_path "${CMAKE_CURRENT_BINARY_DIR}/zucchini-reports/${ZUCCHINIFY_FIXTURE}/$<CONFIG>")
        endif()
        target_compile_definitions(${target} PRIVATE
            ZUCCHINI_DEFAULT_FEATURE_DIR="${ZUCCHINIFY_FEATURE_DIR}"
            ZUCCHINI_DEFAULT_MANIFEST_DIR="${manifest_dir}"
            ZUCCHINI_DEFAULT_RULE_REPORT_DIR="${rule_report_path}")
        list(APPEND rule_coverage_args "--rule-coverage-report=${rule_report_path}")
    endif()

    if(ZUCCHINIFY_UNJUSTIFIED_ASSERTION_COVERAGE)
        target_compile_definitions(${target} PRIVATE
            ZUCCHINI_DEFAULT_UNJUSTIFIED_ASSERTIONS="${unjustified_assertion_severity}"
            ZUCCHINI_DEFAULT_MAX_UNJUSTIFIED_FACTS_PER_SCENARIO="${ZUCCHINIFY_MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO}")
        list(APPEND rule_coverage_args
            "--unjustified-assertions=${unjustified_assertion_severity}"
            "--max-unjustified-facts-per-scenario=${ZUCCHINIFY_MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO}")
    endif()

    # Discovery parses the features and writes the manifests; the run only reads them back.
    # EXTRA_ARGS/DISCOVERY_EXTRA_ARGS are executable arguments; any other arguments are
    # forwarded as gtest_discover_tests options. Put passthrough options before these
    # variadic keywords so cmake_parse_arguments does not consume them as values.
    gtest_discover_tests(${target}
        DISCOVERY_MODE ${ZUCCHINIFY_DISCOVERY_MODE}
        DISCOVERY_EXTRA_ARGS
            "feature_dir=${ZUCCHINIFY_FEATURE_DIR}"
            ${snippet_args}
            ${rule_coverage_args}
            "manifest_dir=${manifest_dir}"
            ${ZUCCHINIFY_DISCOVERY_EXTRA_ARGS}
        EXTRA_ARGS
            "manifest_dir=${manifest_dir}"
            ${ZUCCHINIFY_EXTRA_ARGS}
        ${ZUCCHINIFY_UNPARSED_ARGUMENTS}
    )
endfunction()
