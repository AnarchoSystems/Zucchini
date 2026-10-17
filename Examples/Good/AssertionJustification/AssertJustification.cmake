cmake_minimum_required(VERSION 3.31)

file(REMOVE_RECURSE "${OUTPUT_DIR}")

function(discover name feature_dir expect_success)
    execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
        "feature_dir=${feature_dir}" "manifest_dir=${OUTPUT_DIR}/${name}-manifests"
        "--rule-coverage-report=${OUTPUT_DIR}/${name}" ${ARGN}
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(expect_success AND NOT result EQUAL 0)
        message(FATAL_ERROR "${name}: discovery failed: ${output}${error}")
    elseif(NOT expect_success AND result EQUAL 0)
        message(FATAL_ERROR "${name}: discovery should have failed: ${output}${error}")
    endif()
    # CMake may wrap long diagnostics, particularly with Windows paths.
    string(REGEX REPLACE "[ \t\r\n]+" " " error "${error}")
    set(discovery_error "${error}" PARENT_SCOPE)
    file(READ "${OUTPUT_DIR}/${name}/rules.json" report)
    set(report "${report}" PARENT_SCOPE)
endfunction()

function(expect_json expected)
    string(JSON actual GET "${report}" ${ARGN})
    if(NOT actual STREQUAL "${expected}")
        message(FATAL_ERROR "Expected ${ARGN} to be '${expected}', got '${actual}': ${report}")
    endif()
endfunction()

function(expect_error fragment)
    string(FIND "${discovery_error}" "${fragment}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Expected '${fragment}' in: ${discovery_error}")
    endif()
endfunction()

# The defaults from zucchinify: unjustified facts warn, and each scenario may
# assert at most two of them.
discover(passing "${FEATURE_DIR}" TRUE)
expect_error("Basket.feature:13:5: warning: no business rule justifies 2 asserted table cells: 'note' in row 1, 'note' in row 2")
expect_error("Basket.feature:22:5: warning: no business rule justifies this assertion")
expect_json(2.1.0 schema_version)
expect_json(ON assertion_justification enforced)
expect_json(warning assertion_justification severity)
expect_json(2 assertion_justification max_unjustified_facts_per_scenario)
expect_json(2 assertion_justification scenarios)
expect_json(11 assertion_justification asserted_facts)
expect_json(3 assertion_justification unjustified_facts)
expect_json(0 assertion_justification scenarios_over_limit)
# Empty cells may assert absence; the ignored shelf column asserts nothing.
expect_json(9 scenarios 0 asserted_facts)
expect_json(shelf scenarios 0 steps 4 argument ignoredKeys 0)
expect_json(note scenarios 0 unjustified_facts 1 cell key)
expect_json(1 scenarios 0 unjustified_facts 1 cell row)
expect_json(16 scenarios 0 unjustified_facts 1 line)
expect_json(44 scenarios 0 unjustified_facts 1 column)
expect_json(4 scenarios 1 unjustified_facts 0 step_index)
expect_json(outcome scenarios 1 steps 4 keywordType)
# The rule names the subtotal field; the table spells its header "total".
expect_json(total rules 0 events 2 cell key)
expect_json(15 rules 0 events 2 cell line)
expect_json(28 rules 0 events 2 cell column)

execute_process(COMMAND "${EXECUTABLE}" "manifest_dir=${OUTPUT_DIR}/passing-manifests"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Justification must not affect execution: ${output}${error}")
endif()

# Command-line arguments override the build's defaults.
discover(rejected "${FEATURE_DIR}" FALSE "--unjustified-assertions=error")
expect_error("Basket.feature:22:5: error: no business rule justifies this assertion")
expect_json(error assertion_justification severity)
expect_json(OFF scenarios 1 accepted)

# The limit applies to each scenario, whatever the severity.
discover(over-limit "${FEATURE_DIR}" FALSE "--max-unjustified-facts-per-scenario=1")
expect_error("Basket.feature:10:0: error: scenario asserts 2 facts that no business rule justifies; at most 1 are allowed")
expect_json(1 assertion_justification scenarios_over_limit)
expect_json(OFF scenarios 0 accepted)
expect_json(ON scenarios 1 accepted)
expect_json(OFF discovery_success)

# '*' makes no claim about being an assertion, so this scenario asserts nothing.
file(MAKE_DIRECTORY "${OUTPUT_DIR}/asterisk-input")
file(WRITE "${OUTPUT_DIR}/asterisk-input/scenario.feature"
    "Feature: Neutral steps\n  Scenario: Asterisks assert nothing\n    * each apple costs 3 credits\n    * I put 1 apple into my basket\n    * my basket total is 3 credits\n")
discover(asterisk "${OUTPUT_DIR}/asterisk-input" FALSE)
expect_error("scenario.feature:2:0: error: scenario asserts nothing; add an outcome step such as Then")
expect_json(0 assertion_justification asserted_facts)
expect_json(1 assertion_justification scenarios_without_assertions)
