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
if(NOT schema EQUAL 2)
    message(FATAL_ERROR "Expected requirement report schema version 2")
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
# A filtered test run updates only that scenario, with no JUnit command-line option.
string(JSON first_name GET "${json}" scenarios 0 test_name)
string(JSON first_key GET "${json}" scenarios 0 result_key)
string(JSON second_key GET "${json}" scenarios 1 result_key)
string(JSON generation GET "${json}" discovery_id)
execute_process(COMMAND "${EXECUTABLE}" "--gtest_filter=Notes.${first_name}"
    "manifest_dir=${OUTPUT_DIR}/manifests" "--rule-coverage-report=${report_dir}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Filtered execution failed: ${output}${error}")
endif()
file(READ "${report_dir}/results/${first_key}.json" execution)
string(JSON status GET "${execution}" status)
string(JSON result_generation GET "${execution}" discovery_id)
if(NOT status STREQUAL "passed" OR NOT result_generation STREQUAL generation OR
   NOT EXISTS "${report_dir}/results/${first_key}.xml")
    message(FATAL_ERROR "Missing execution results: ${execution}")
endif()
file(READ "${report_dir}/results/${second_key}.json" untouched)
string(JSON status GET "${untouched}" status)
if(NOT status STREQUAL "not_run")
    message(FATAL_ERROR "Filtered execution replaced another scenario's result")
endif()

# Direct Test Explorer-style invocation uses the defaults embedded by zucchinify.
file(READ "${CONFIGURED_REPORT_DIR}/rules.json" configured_discovery)
string(JSON configured_generation GET "${configured_discovery}" discovery_id)
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Discovery without extra runner arguments failed: ${output}${error}")
endif()
file(READ "${CONFIGURED_REPORT_DIR}/rules.json" configured_discovery)
string(JSON refreshed_generation GET "${configured_discovery}" discovery_id)
if(NOT refreshed_generation STREQUAL configured_generation)
    message(FATAL_ERROR "Unchanged discovery invalidated execution results")
endif()
execute_process(COMMAND "${EXECUTABLE}" "--gtest_filter=Notes.${first_name}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Execution without extra runner arguments failed: ${output}${error}")
endif()
file(READ "${CONFIGURED_REPORT_DIR}/results/${first_key}.json" configured_execution)
string(JSON status GET "${configured_execution}" status)
if(NOT status STREQUAL "passed")
    message(FATAL_ERROR "Configured report directory did not receive the result")
endif()

# Runtime assertion failures are recorded, independently of discovery violations.
file(MAKE_DIRECTORY "${OUTPUT_DIR}/failing")
file(WRITE "${OUTPUT_DIR}/failing/scenario.feature"
    "Feature: Execution failure\n  Scenario: ExpectedFailure\n    Given I start with 0\n    Then the result is 999\n")
set(failure_report "${OUTPUT_DIR}/failure report")
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${OUTPUT_DIR}/failing" "manifest_dir=${OUTPUT_DIR}/failure-manifests"
    "--rule-coverage-report=${failure_report}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failure fixture discovery failed: ${output}${error}")
endif()
file(READ "${failure_report}/rules.json" failure_json)
string(JSON failing_name GET "${failure_json}" scenarios 0 test_name)
string(JSON failing_key GET "${failure_json}" scenarios 0 result_key)
execute_process(COMMAND "${EXECUTABLE}" "--gtest_filter=Notes.${failing_name}"
    "manifest_dir=${OUTPUT_DIR}/failure-manifests" "--rule-coverage-report=${failure_report}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0)
    message(FATAL_ERROR "Expected runtime assertion failure")
endif()
file(READ "${failure_report}/results/${failing_key}.json" execution)
string(JSON status GET "${execution}" status)
string(JSON source GET "${execution}" messages 0 gherkin_source)
if(NOT status STREQUAL "failed" OR source STREQUAL "" OR
   NOT EXISTS "${failure_report}/results/${failing_key}.xml")
    message(FATAL_ERROR "Missing runtime failure information: ${execution}")
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
if(NOT schema EQUAL 2)
    message(FATAL_ERROR "Expected requirement report schema version 2")
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
if(NOT schema EQUAL 2)
    message(FATAL_ERROR "Expected requirement report schema version 2")
endif()
string(JSON success GET "${json}" discovery_success)
string(JSON count LENGTH "${json}" discovery_diagnostics)
if(result EQUAL 0 OR success OR count EQUAL 0)
    message(FATAL_ERROR "Parse failure did not update the dashboard: ${json}")
endif()
