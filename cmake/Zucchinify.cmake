include_guard(GLOBAL)

include(GoogleTest)

# Generates the fixture sources for <FIXTURE> from the single YAML manifest in <FEATURE_DIR>,
# attaches them to <target> and wires gtest discovery to the feature files.
function(zucchinify target)
    cmake_parse_arguments(ZUCCHINIFY "" "FEATURE_DIR;FIXTURE;DISCOVERY_MODE" "" ${ARGN})

    if(NOT ZUCCHINIFY_FEATURE_DIR OR NOT ZUCCHINIFY_FIXTURE)
        message(FATAL_ERROR "zucchinify(${target}) requires FEATURE_DIR and FIXTURE")
    endif()
    if(NOT ZUCCHINIFY_DISCOVERY_MODE)
        set(ZUCCHINIFY_DISCOVERY_MODE POST_BUILD)
    endif()

    file(GLOB manifests CONFIGURE_DEPENDS
        "${ZUCCHINIFY_FEATURE_DIR}/*.yaml"
        "${ZUCCHINIFY_FEATURE_DIR}/*.yml")
    list(LENGTH manifests manifest_count)
    if(NOT manifest_count EQUAL 1)
        message(FATAL_ERROR
            "zucchinify(${target}): expected exactly one .yaml/.yml in ${ZUCCHINIFY_FEATURE_DIR}, found ${manifest_count}")
    endif()
    list(GET manifests 0 manifest)

    # Feature files are consumed by gtest discovery, not by the generator itself. Keep them
    # as object dependencies so editing a feature causes the test target to relink and thus
    # rerun POST_BUILD discovery. LINK_DEPENDS is not honored consistently by all generators.
    file(GLOB_RECURSE features CONFIGURE_DEPENDS "${ZUCCHINIFY_FEATURE_DIR}/*.feature")

    set(generated "${CMAKE_CURRENT_BINARY_DIR}/zucchini-generated/${ZUCCHINIFY_FIXTURE}")
    set(manifest_dir "${CMAKE_CURRENT_BINARY_DIR}/zucchini-manifests/${ZUCCHINIFY_FIXTURE}")
    set(header "${generated}/I${ZUCCHINIFY_FIXTURE}.h")
    set(test_source "${generated}/${ZUCCHINIFY_FIXTURE}Test.cc")

    add_custom_command(
        OUTPUT "${header}" "${test_source}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${generated}"
        COMMAND $<TARGET_FILE:Zucchini> -i "${manifest}" -fixture "${ZUCCHINIFY_FIXTURE}" -o "${generated}"
        DEPENDS Zucchini "${manifest}"
        COMMENT "Zucchini: generating ${ZUCCHINIFY_FIXTURE} fixture"
        VERBATIM
    )

    # Discovery consumes feature files after the test binary is built. Give the target a tiny
    # generated source whose timestamp tracks those files, so every generator rebuilds and
    # relinks the executable without making the Zucchini generator depend on feature files.
    set(feature_trigger "${generated}/${ZUCCHINIFY_FIXTURE}FeatureDependencies.cc")
    add_custom_command(
        OUTPUT "${feature_trigger}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${feature_trigger}"
        DEPENDS ${features}
        COMMENT "Zucchini: tracking ${ZUCCHINIFY_FIXTURE} feature changes"
        VERBATIM
    )

    target_sources(${target} PRIVATE "${test_source}" "${header}" "${feature_trigger}")
    target_include_directories(${target} PRIVATE "${generated}" "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${target} PRIVATE Zucchini::LibZucchini)

    # Discovery parses the features and writes the manifests; the run only reads them back.
    gtest_discover_tests(${target}
        DISCOVERY_MODE ${ZUCCHINIFY_DISCOVERY_MODE}
        DISCOVERY_EXTRA_ARGS
            "feature_dir=${ZUCCHINIFY_FEATURE_DIR}"
            "manifest_dir=${manifest_dir}"
        EXTRA_ARGS
            "manifest_dir=${manifest_dir}"
    )
endfunction()
