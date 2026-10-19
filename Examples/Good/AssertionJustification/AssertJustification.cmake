cmake_minimum_required(VERSION 3.31)

file(REMOVE_RECURSE "${OUTPUT_DIR}")

function(discover name feature_dir)
    execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
        "feature_dir=${feature_dir}" "manifest_dir=${OUTPUT_DIR}/${name}-manifests"
        "--rule-coverage-report=${OUTPUT_DIR}/${name}" ${ARGN}
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${name}: discovery failed: ${output}${error}")
    endif()
    # Assertion statistics are advisory: discovery diagnoses nothing for them.
    if(error MATCHES "justif")
        message(FATAL_ERROR "${name}: unexpected justification diagnostics: ${error}")
    endif()
    file(READ "${OUTPUT_DIR}/${name}/rules.json" report)
    set(report "${report}" PARENT_SCOPE)
endfunction()

function(expect_json expected)
    string(JSON actual GET "${report}" ${ARGN})
    if(NOT actual STREQUAL "${expected}")
        message(FATAL_ERROR "Expected ${ARGN} to be '${expected}', got '${actual}': ${report}")
    endif()
endfunction()

discover(passing "${FEATURE_DIR}")
expect_json(2.2.0 schema_version)
expect_json(OFF assertion_justification enforced)
expect_json(2 assertion_justification scenarios)
expect_json(0 assertion_justification scenarios_without_assertions)
expect_json(11 assertion_justification asserted_facts)
expect_json(3 assertion_justification unjustified_facts)
string(JSON severity ERROR_VARIABLE missing GET "${report}" assertion_justification severity)
if(NOT missing)
    message(FATAL_ERROR "Deprecated settings must not be written: ${report}")
endif()
# Each requirement declares how narrowly it pins down outcomes.
expect_json(exact rules 0 requirements 0 strictness)
expect_json(exact rules 0 requirements 2 strictness)
# Empty cells may assert absence; the ignored shelf column asserts nothing.
expect_json(9 scenarios 0 asserted_facts)
expect_json(shelf scenarios 0 steps 4 argument ignoredKeys 0)
expect_json(note scenarios 0 unjustified_facts 1 cell key)
expect_json(1 scenarios 0 unjustified_facts 1 cell row)
expect_json(16 scenarios 0 unjustified_facts 1 line)
expect_json(44 scenarios 0 unjustified_facts 1 column)
expect_json(4 scenarios 1 unjustified_facts 0 step_index)
expect_json(outcome scenarios 1 steps 4 keywordType)
expect_json(ON scenarios 0 accepted)
expect_json(ON scenarios 1 accepted)
# The rule names the subtotal field; the table spells its header "total".
expect_json(total rules 0 events 2 cell key)
expect_json(15 rules 0 events 2 cell line)
expect_json(28 rules 0 events 2 cell column)

execute_process(COMMAND "${EXECUTABLE}" "manifest_dir=${OUTPUT_DIR}/passing-manifests"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Assertion statistics must not affect execution: ${output}${error}")
endif()

# Removed enforcement options are ignored.
discover(removed-options "${FEATURE_DIR}" "--unjustified-assertions=error"
    "--max-unjustified-facts-per-scenario=0")
expect_json(ON scenarios 1 accepted)
expect_json(ON discovery_success)

# '*' makes no claim about being an assertion, so this scenario asserts nothing.
# It is counted, not rejected.
file(MAKE_DIRECTORY "${OUTPUT_DIR}/asterisk-input")
file(WRITE "${OUTPUT_DIR}/asterisk-input/scenario.feature"
    "Feature: Neutral steps\n  Scenario: Asterisks assert nothing\n    * each apple costs 3 credits\n    * I put 1 apple into my basket\n    * my basket total is 3 credits\n")
discover(asterisk "${OUTPUT_DIR}/asterisk-input")
expect_json(0 assertion_justification asserted_facts)
expect_json(1 assertion_justification scenarios_without_assertions)
expect_json(ON scenarios 0 accepted)

# Without the HTML report, discovery writes rules.json and removes the report
# files it wrote earlier.
execute_process(COMMAND "${EXECUTABLE}" --gtest_list_tests
    "feature_dir=${FEATURE_DIR}" "manifest_dir=${OUTPUT_DIR}/passing-manifests"
    "--rule-report-data=${OUTPUT_DIR}/passing"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Data-only discovery failed: ${output}${error}")
endif()
if(NOT EXISTS "${OUTPUT_DIR}/passing/rules.json" OR EXISTS "${OUTPUT_DIR}/passing/index.html")
    message(FATAL_ERROR "Data-only discovery must write rules.json without the HTML report")
endif()

# The build's report loads the stylesheet given to zucchinify after its own styles.
file(READ "${BUILD_REPORT_DIR}/custom.css" css)
if(NOT css MATCHES "--strictness-exact: #c8ecd9")
    message(FATAL_ERROR "The report stylesheet was not copied: ${css}")
endif()
file(READ "${BUILD_REPORT_DIR}/index.html" html)
if(NOT html MATCHES "<link rel=\"stylesheet\" href=\"custom.css\">")
    message(FATAL_ERROR "The report does not load its stylesheet")
endif()
