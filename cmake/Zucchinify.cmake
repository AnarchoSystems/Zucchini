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

    target_sources(${target} PRIVATE "${test_source}" "${header}")
    target_include_directories(${target} PRIVATE "${generated}" "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${target} PRIVATE Zucchini::LibZucchini)

    # Re-link (and therefore re-discover) whenever a feature file changes.
    file(GLOB_RECURSE features CONFIGURE_DEPENDS "${ZUCCHINIFY_FEATURE_DIR}/*.feature")
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS ${features})

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
