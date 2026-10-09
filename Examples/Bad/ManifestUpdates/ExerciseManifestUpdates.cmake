file(REMOVE_RECURSE "${TEST_BINARY_DIR}")
set(template_source "${TEST_SOURCE_DIR}")

# Generator editing cases update source fixtures; the parent restores their backups.
set(case_dir "${TEST_BINARY_DIR}/editing cases")
file(MAKE_DIRECTORY "${case_dir}")
set(suggestions "${case_dir}/suggestions.yaml")
file(WRITE "${suggestions}" "steps: [{step: '^added$', methodName: added}]\n")
function(check_exact name)
    file(READ "${template_source}/editing/${name}/after.yml" after)
    set(path "${TEST_SOURCE_DIR}/editing/${name}/before.yml")
    execute_process(COMMAND "${ZUCCHINI_EXECUTABLE}" -update "${path}" -suggestions "${suggestions}"
        RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT result EQUAL 0 OR NOT out STREQUAL "" OR NOT err STREQUAL "")
        message(FATAL_ERROR "${name} update failed: ${out}${err}")
    endif()
    configure_file("${template_source}/editing/${name}/after.yml" "${case_dir}/${name}.expected" COPYONLY)
    file(READ "${path}" actual HEX)
    file(READ "${case_dir}/${name}.expected" expected HEX)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "${name} source preservation failed:\n${actual}\nExpected:\n${after}")
    endif()
    execute_process(COMMAND "${ZUCCHINI_EXECUTABLE}" -update "${path}" -suggestions "${suggestions}"
        RESULT_VARIABLE result)
    file(READ "${path}" repeated HEX)
    if(NOT result EQUAL 0 OR NOT repeated STREQUAL actual)
        message(FATAL_ERROR "${name} update was not idempotent")
    endif()
endfunction()
foreach(name IN ITEMS empty_flow populated_flow trailing_comma flow_root already_in_group)
    check_exact(${name})
endforeach()
# Invalid suggestions must leave the user's bytes unchanged.
file(WRITE "${suggestions}" "steps: [{step: '^bad$', methodName: class}]\n")
set(path "${TEST_SOURCE_DIR}/editing/invalid/before.yml")
execute_process(COMMAND "${ZUCCHINI_EXECUTABLE}" -update "${path}" -suggestions "${suggestions}"
    RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
file(READ "${path}" actual)
if(result EQUAL 0 OR NOT actual STREQUAL "# untouched\nsteps: []\n")
    message(FATAL_ERROR "Invalid suggestions changed the manifest")
endif()
# New table types, plus colliding names, must produce a valid generated API.
file(WRITE "${suggestions}" "types:\n  - name: AddedRow\n    kind: struct\n    fields: [{name: values, type: list, content: int, optional: true}]\nsteps:\n  - step: '^added$'\n    methodName: added\n    dataTable: {type: AddedRow}\n")
foreach(name IN ITEMS missing_types empty_types existing_types flow_root_types flow_root_trailing_comma)
    set(path "${TEST_SOURCE_DIR}/editing/${name}/before.yml")
    execute_process(COMMAND "${ZUCCHINI_EXECUTABLE}" -update "${path}" -suggestions "${suggestions}"
        RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${name} update failed: ${out}${err}")
    endif()
    file(READ "${path}" actual)
    file(READ "${template_source}/editing/${name}/after.yml" expected)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "${name} differs from after.yml: ${actual}")
    endif()
    if(NOT actual MATCHES "keep" OR NOT actual MATCHES "AddedRow")
        message(FATAL_ERROR "${name} lost comments or table types")
    endif()
    if(name STREQUAL existing_types)
        string(FIND "${actual}" "    - {name: AddedRow, kind: struct, fields: [{name: original}]} # preserved" found)
        if(found LESS 0 OR NOT actual MATCHES "AddedRow2" OR NOT actual MATCHES "added2")
            message(FATAL_ERROR "Colliding names or existing indentation were not preserved: ${actual}")
        endif()
    endif()
    execute_process(COMMAND "${ZUCCHINI_EXECUTABLE}" -i "${path}" -fixture Verify
        -o "${case_dir}/${name}" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${name} updated manifest does not generate: ${out}${err}")
    endif()
    execute_process(COMMAND "${ZUCCHINI_EXECUTABLE}" -update "${path}" -suggestions "${suggestions}"
        RESULT_VARIABLE result)
    file(READ "${path}" repeated)
    if(NOT result EQUAL 0 OR NOT repeated STREQUAL actual)
        message(FATAL_ERROR "${name} duplicated suggested types or steps")
    endif()
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${TEST_SOURCE_DIR}"
        -B "${TEST_BINARY_DIR}"
        -D "CMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}"
        -D "CMAKE_CXX_FLAGS=${TEST_CXX_FLAGS}"
        -D "ZUCCHINI_PROJECT_SOURCE_DIR=${ZUCCHINI_PROJECT_SOURCE_DIR}"
        -D "BAD_EXAMPLE_SUPPORT_DIR=${BAD_EXAMPLE_SUPPORT_DIR}"
        -D "ZUCCHINI_EXECUTABLE=${ZUCCHINI_EXECUTABLE}"
        -D "ZUCCHINI_LIBRARY=${ZUCCHINI_LIBRARY}"
        -D "YAML_LIBRARY=${YAML_LIBRARY}"
        -D "ZUCCHINI_RUNTIME_LIBRARY=${ZUCCHINI_RUNTIME_LIBRARY}"
        -D "CUCUMBER_GHERKIN_LIBRARY=${CUCUMBER_GHERKIN_LIBRARY}"
        -D "CUCUMBER_GHERKIN_INCLUDE_DIRS=${CUCUMBER_GHERKIN_INCLUDE_DIRS}"
        -D "CUCUMBER_MESSAGES_LIBRARY=${CUCUMBER_MESSAGES_LIBRARY}"
        -D "CUCUMBER_MESSAGES_INCLUDE_DIRS=${CUCUMBER_MESSAGES_INCLUDE_DIRS}"
        -D "GTEST_LIBRARY=${GTEST_LIBRARY}"
        -D "GTEST_INCLUDE_DIRS=${GTEST_INCLUDE_DIRS}"
        -D "JSON_INCLUDE_DIRS=${JSON_INCLUDE_DIRS}"
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR
        "PRE_TEST example failed to configure:\n${configure_stdout}${configure_stderr}")
endif()

function(build_example)
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${TEST_BINARY_DIR}"
        --config "${TEST_BUILD_CONFIG}" --target ManifestUpdatesTest
        RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Build failed: ${out}${err}")
    endif()
endfunction()
include("${BAD_EXAMPLE_SUPPORT_DIR}/ManifestReviewUrl.cmake")
manifest_review_url(review_url "${TEST_SOURCE_DIR}/features/manifest.yaml")
function(discover expected_success)
    execute_process(COMMAND "${CTEST_EXECUTABLE}" --test-dir "${TEST_BINARY_DIR}"
        --build-config "${TEST_BUILD_CONFIG}" --output-on-failure
        RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(expected_success)
        if(NOT result EQUAL 0 OR NOT out MATCHES "100% tests passed")
            message(FATAL_ERROR "Updated example failed: ${out}${err}")
        endif()
    else()
        # CMake's discovery error formatting wraps lines and may add sentence
        # spacing. Assert the diagnostic content rather than that presentation.
        string(REGEX REPLACE "[ \t\r\n]+" " " diagnostic "${out}${err}")
        string(FIND "${diagnostic}" "undefined step definitions. please review ${review_url}" found)
        if(result EQUAL 0 OR found LESS 0)
            message(FATAL_ERROR "Expected update-and-review URL ${review_url}: ${out}${err}")
        endif()
    endif()
endfunction()
set(manifest "${TEST_SOURCE_DIR}/features/manifest.yaml")
file(READ "${manifest}" original)
build_example()
discover(FALSE)
file(READ "${manifest}" updated)
file(READ "${template_source}/after.yml" expected)
if(NOT updated STREQUAL expected)
    message(FATAL_ERROR "Discovery manifest differs from after.yml: ${updated}")
endif()
discover(FALSE)
file(READ "${manifest}" repeated)
if(NOT repeated STREQUAL updated)
    message(FATAL_ERROR "Stale discovery duplicated or rewrote definitions")
endif()
build_example()
discover(TRUE)
