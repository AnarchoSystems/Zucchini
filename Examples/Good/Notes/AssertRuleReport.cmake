cmake_minimum_required(VERSION 3.27)

file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
set(report_dir "${OUTPUT_DIR}/coverage report")
set(report "${report_dir}/rules.json")
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${FEATURE_DIR}" "manifest_dir=${OUTPUT_DIR}/manifests"
    "--rule-coverage-report=${report_dir}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Discovery failed: ${output}${error}")
endif()
if(NOT EXISTS "${report_dir}/index.html" OR NOT EXISTS "${report_dir}/rules.js" OR NOT EXISTS "${report_dir}/dashboard.js")
    message(FATAL_ERROR "Missing dashboard assets")
endif()
file(READ "${report}" json)
string(JSON schema GET "${json}" schema_version)
if(schema VERSION_LESS 2 OR NOT schema VERSION_LESS 3)
    message(FATAL_ERROR "Expected requirement report schema version 2.x, got ${schema}")
endif()
string(JSON kind GET "${json}" rules 0 kind)
string(JSON tag GET "${json}" rules 0 events 0 tag)
string(JSON event_kind GET "${json}" rules 0 events 0 kind)
string(JSON accepted GET "${json}" rules 0 events 0 scenario_accepted)
string(JSON test_name GET "${json}" rules 0 events 0 test_name)
if(NOT accepted OR test_name STREQUAL "")
    message(FATAL_ERROR "Missing test identity: ${json}")
endif()
string(JSON requirement_count LENGTH "${json}" rules 0 requirements)
set(tag_declared FALSE)
if(requirement_count GREATER 0)
    math(EXPR last_requirement "${requirement_count} - 1")
    foreach(index RANGE 0 ${last_requirement})
        string(JSON declared_tag GET "${json}" rules 0 requirements ${index} tag)
        if("${tag}" STREQUAL "${declared_tag}")
            set(tag_declared TRUE)
        endif()
    endforeach()
endif()
if(NOT kind STREQUAL "business" OR NOT tag_declared OR NOT event_kind STREQUAL "coverage")
    message(FATAL_ERROR "Unexpected rule report: ${json}")
endif()
# Runtime tests must never read or update the discovery report.
string(JSON first_name GET "${json}" scenarios 0 test_name)
file(SHA256 "${report}" before)
execute_process(COMMAND "${EXECUTABLE}" "--gtest_filter=Notes.${first_name}"
    "manifest_dir=${OUTPUT_DIR}/manifests" "--rule-coverage-report=${report_dir}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(SHA256 "${report}" after)
if(NOT result EQUAL 0 OR NOT before STREQUAL after OR EXISTS "${report_dir}/results")
    message(FATAL_ERROR "Execution must leave discovery artifacts untouched: ${output}${error}")
endif()
# Even an unusable report path must not affect execution.
file(WRITE "${OUTPUT_DIR}/not-a-directory" "file")
execute_process(COMMAND "${EXECUTABLE}" "--gtest_filter=Notes.${first_name}"
    "manifest_dir=${OUTPUT_DIR}/manifests"
    "--rule-coverage-report=${OUTPUT_DIR}/not-a-directory"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Runtime accessed the report path: ${output}${error}")
endif()

# A rejected scenario must still produce its violations in the report.
file(MAKE_DIRECTORY "${OUTPUT_DIR}/bad")
file(WRITE "${OUTPUT_DIR}/bad/scenario.feature"
    "Feature: Invalid\n  Scenario: Note before setup\n    Given I note\n      \"\"\"\n      note\n      \"\"\"\n")
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${OUTPUT_DIR}/bad" "manifest_dir=${OUTPUT_DIR}/bad-manifests"
    "--rule-coverage-report=${report_dir}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0)
    message(FATAL_ERROR "Invalid scenario was accepted")
endif()
file(READ "${report}" json)
string(JSON schema GET "${json}" schema_version)
if(schema VERSION_LESS 2 OR NOT schema VERSION_LESS 3)
    message(FATAL_ERROR "Expected requirement report schema version 2.x, got ${schema}")
endif()
string(JSON event_kind GET "${json}" rules 0 events 1 kind)
string(JSON severity GET "${json}" rules 0 events 1 severity)
string(JSON line GET "${json}" rules 0 events 1 line)
if(NOT event_kind STREQUAL "violation" OR NOT severity STREQUAL "error" OR NOT line EQUAL 3)
    message(FATAL_ERROR "Missing violation: ${json}")
endif()

# Parse failures replace the dashboard's discovery data rather than leaving old success visible.
file(WRITE "${OUTPUT_DIR}/bad/scenario.feature" "This is not Gherkin\n")
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${OUTPUT_DIR}/bad" "manifest_dir=${OUTPUT_DIR}/bad-manifests"
    "--rule-coverage-report=${report_dir}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(READ "${report}" json)
string(JSON schema GET "${json}" schema_version)
if(schema VERSION_LESS 2 OR NOT schema VERSION_LESS 3)
    message(FATAL_ERROR "Expected requirement report schema version 2.x, got ${schema}")
endif()
string(JSON success GET "${json}" discovery_success)
string(JSON count LENGTH "${json}" discovery_diagnostics)
if(result EQUAL 0 OR success OR count EQUAL 0)
    message(FATAL_ERROR "Parse failure did not update the dashboard: ${json}")
endif()
