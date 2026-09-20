function(add_bad_example name)
    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
    set(binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
    set(configure_arguments
        -D "TEST_SOURCE_DIR=${source_dir}"
        -D "TEST_BINARY_DIR=${binary_dir}"
        -D "ZUCCHINI_EXECUTABLE=$<TARGET_FILE:Zucchini>"
        -D "BAD_EXAMPLE_SUPPORT_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        -D "ZUCCHINI_PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
        -D "ZUCCHINI_BUILD_DIR=${PROJECT_BINARY_DIR}"
        -D "ZUCCHINI_LIBRARY=$<TARGET_FILE:LibZucchini>"
        -D "CUCUMBER_GHERKIN_LIBRARY=$<TARGET_FILE:cucumber::gherkin>"
        -D "CUCUMBER_MESSAGES_LIBRARY=$<TARGET_FILE:cucumber::messages>"
        -D "GTEST_LIBRARY=$<TARGET_FILE:GTest::gtest>"
        -D "JSON_SCHEMA_VALIDATOR_LIBRARY=$<TARGET_FILE:nlohmann_json_schema_validator::validator>")

    add_test(NAME "Bad.${name}"
        COMMAND "${CMAKE_COMMAND}"
            ${configure_arguments}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/RunBadExample.cmake")
endfunction()

add_bad_example(MissingRequiredField)
add_bad_example(MalformedYaml)
add_bad_example(UnanchoredStepRegex)
add_bad_example(InvalidMethodName)
add_bad_example(CppKeywordIdentifier)
add_bad_example(ValidateScenario)
add_bad_example(TestAssertion)
add_bad_example(InvalidEnumValue)
add_bad_example(MissingRequiredTableField)
add_bad_example(TableHeaderTypo)
add_bad_example(MalformedTable)
add_bad_example(MalformedTypedDocString)
add_bad_example(WrongTypedDocStringValue)
add_bad_example(InvalidRegexArgument)