file(REMOVE_RECURSE "${OUTPUT_DIR}")
set(report_dir "${OUTPUT_DIR}/passing")
set(manifests "${OUTPUT_DIR}/manifests")
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${FEATURE_DIR}" "manifest_dir=${manifests}"
    "--rule-coverage-report=${report_dir}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Business demo discovery failed: ${output}${error}")
endif()
execute_process(COMMAND "${EXECUTABLE}" "manifest_dir=${manifests}"
    "--rule-coverage-report=${report_dir}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Business demo execution failed: ${output}${error}")
endif()
file(READ "${report_dir}/rules.json" json)

# Reports record unjustified assertions without enforcing them: no rule claims
# "the purchase is accepted" as coverage.
string(JSON enforced GET "${json}" assertion_justification enforced)
string(JSON unjustified GET "${json}" scenarios 0 unjustified_facts 0 step_index)
string(JSON unjustified_step GET "${json}" scenarios 0 steps ${unjustified} text)
if(enforced OR NOT unjustified_step STREQUAL "the purchase is accepted")
    message(FATAL_ERROR "Expected unenforced, reported unjustified assertions: ${json}")
endif()

# Requirement identities are enum values; human-readable tags may be edited.
function(requirement_tag rule_index requirement_value output)
    string(JSON count LENGTH "${json}" rules ${rule_index} requirements)
    if(count GREATER 0)
        math(EXPR last "${count} - 1")
        foreach(index RANGE 0 ${last})
            string(JSON value GET "${json}" rules ${rule_index} requirements ${index} value)
            if(value EQUAL requirement_value)
                string(JSON tag GET "${json}" rules ${rule_index} requirements ${index} tag)
                set(${output} "${tag}" PARENT_SCOPE)
                return()
            endif()
        endforeach()
    endif()
    message(FATAL_ERROR "Rule ${rule_index} is missing requirement ${requirement_value}")
endfunction()

function(count_coverage rule_index requirement_value output)
    requirement_tag(${rule_index} ${requirement_value} tag)
    string(JSON count LENGTH "${json}" rules ${rule_index} events)
    set(found 0)
    if(count GREATER 0)
        math(EXPR last "${count} - 1")
        foreach(index RANGE 0 ${last})
            string(JSON kind GET "${json}" rules ${rule_index} events ${index} kind)
            string(JSON event_tag GET "${json}" rules ${rule_index} events ${index} tag)
            if(kind STREQUAL "coverage" AND event_tag STREQUAL "${tag}")
                math(EXPR found "${found} + 1")
            endif()
        endforeach()
    endif()
    set(${output} ${found} PARENT_SCOPE)
endfunction()

count_coverage(1 2 wallet_refund)
count_coverage(2 2 inventory_refund)
count_coverage(1 0 purchases)
count_coverage(1 1 rejections)
if(NOT wallet_refund EQUAL 0 OR NOT inventory_refund EQUAL 1 OR
   NOT purchases EQUAL 2 OR NOT rejections EQUAL 2)
    message(FATAL_ERROR "Coverage must reflect asserted invariants, not operations: ${json}")
endif()
string(JSON count LENGTH "${json}" scenarios)
if(NOT count EQUAL 4)
    message(FATAL_ERROR "Expected four scenarios")
endif()
if(EXISTS "${report_dir}/results")
    message(FATAL_ERROR "Runtime must not write rule execution results")
endif()

# Incorrect balance assertions violate the business rule at discovery time.
file(MAKE_DIRECTORY "${OUTPUT_DIR}/invalid")
file(WRITE "${OUTPUT_DIR}/invalid/scenario.feature"
    "Feature: Invalid debit expectation\n  Scenario: Wrong wallet arithmetic\n    Given my wallet has 50 credits\n    And the shop has 5 items\n    And each item costs 7 credits\n    When I buy 2 items\n    Then the purchase is accepted\n    And my wallet balance is 37 credits\n    And the remaining stock is 3\n")
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${OUTPUT_DIR}/invalid" "manifest_dir=${OUTPUT_DIR}/invalid-manifests"
    "--rule-coverage-report=${OUTPUT_DIR}/rejected"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0)
    message(FATAL_ERROR "Incorrect debit expectation was accepted")
endif()
file(READ "${OUTPUT_DIR}/rejected/rules.json" json)
string(JSON tag GET "${json}" rules 1 events 0 tag)
string(JSON kind GET "${json}" rules 1 events 0 kind)
string(JSON line GET "${json}" rules 1 events 0 line)
requirement_tag(1 0 expected_tag)
if(NOT tag STREQUAL "${expected_tag}" OR NOT kind STREQUAL "violation" OR NOT line EQUAL 8)
    message(FATAL_ERROR "Expected a wallet business violation at the assertion: ${json}")
endif()

# The independent rules check conditional effects, not the entire eligibility
# policy. A mistaken rejection expectation can be coherent but fail at runtime.
file(MAKE_DIRECTORY "${OUTPUT_DIR}/failing")
file(WRITE "${OUTPUT_DIR}/failing/scenario.feature"
    "Feature: Runtime checks complement partial models\n  Scenario: Rejection expected despite available funds and stock\n    Given my wallet has 50 credits\n    And the shop has 5 items\n    And each item costs 7 credits\n    When I buy 2 items\n    Then the purchase is rejected\n    And my wallet balance is 50 credits\n    And the remaining stock is 5\n")
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${OUTPUT_DIR}/failing" "manifest_dir=${OUTPUT_DIR}/failing-manifests"
    "--rule-coverage-report=${OUTPUT_DIR}/failing-evidence"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Conditional effects should remain coherent: ${output}${error}")
endif()
execute_process(COMMAND "${EXECUTABLE}" "manifest_dir=${OUTPUT_DIR}/failing-manifests"
    "--rule-coverage-report=${OUTPUT_DIR}/failing-evidence"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0)
    message(FATAL_ERROR "Runtime should reject the mistaken outcome expectation")
endif()
file(READ "${OUTPUT_DIR}/failing-evidence/rules.json" json)
count_coverage(1 1 coverage)
if(NOT coverage EQUAL 1)
    message(FATAL_ERROR "Static coverage must remain distinct from runtime success")
endif()

# End hooks can enforce a completed protocol without enumerating coverage gaps.
file(MAKE_DIRECTORY "${OUTPUT_DIR}/unfinished-input")
file(WRITE "${OUTPUT_DIR}/unfinished-input/scenario.feature"
    "Feature: Unfinished purchase\n  Scenario: Missing outcome assertion\n    Given my wallet has 50 credits\n    And the shop has 5 items\n    And each item costs 7 credits\n    When I buy 2 items\n")
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${OUTPUT_DIR}/unfinished-input" "manifest_dir=${OUTPUT_DIR}/unfinished-manifests"
    "--rule-coverage-report=${OUTPUT_DIR}/unfinished"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0)
    message(FATAL_ERROR "The unfinished purchase protocol was accepted")
endif()
file(READ "${OUTPUT_DIR}/unfinished/rules.json" json)
string(JSON tag GET "${json}" rules 0 events 0 tag)
string(JSON kind GET "${json}" rules 0 events 0 kind)
string(JSON line GET "${json}" rules 0 events 0 line)
requirement_tag(0 6 expected_tag)
if(NOT tag STREQUAL "${expected_tag}" OR NOT kind STREQUAL "violation" OR NOT line EQUAL 2)
    message(FATAL_ERROR "Expected the end hook's DSL violation at the scenario: ${json}")
endif()
