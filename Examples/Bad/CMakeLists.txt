function(add_bad_example name)
    set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
    set(binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
    if(CMAKE_CONFIGURATION_TYPES)
        set(test_build_config "$<CONFIG>")
    else()
        set(test_build_config "")
    endif()
    get_target_property(gherkin_include_dirs cucumber::gherkin INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(messages_include_dirs cucumber::messages INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(json_include_dirs nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(gtest_include_dirs GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(validator_include_dirs nlohmann_json_schema_validator::validator INTERFACE_INCLUDE_DIRECTORIES)
    set(configure_arguments
        -D "TEST_SOURCE_DIR=${source_dir}"
        -D "TEST_BINARY_DIR=${binary_dir}"
        -D "TEST_BUILD_CONFIG=${test_build_config}"
        -D "ZUCCHINI_EXECUTABLE=$<TARGET_FILE:Zucchini>"
        -D "BAD_EXAMPLE_SUPPORT_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        -D "ZUCCHINI_PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
        -D "ZUCCHINI_BUILD_DIR=${PROJECT_BINARY_DIR}"
        -D "ZUCCHINI_LIBRARY=$<TARGET_FILE:LibZucchini>"
        -D "CUCUMBER_GHERKIN_LIBRARY=$<TARGET_FILE:cucumber::gherkin>"
        -D "CUCUMBER_GHERKIN_INCLUDE_DIRS=${gherkin_include_dirs}"
        -D "CUCUMBER_MESSAGES_LIBRARY=$<TARGET_FILE:cucumber::messages>"
        -D "CUCUMBER_MESSAGES_INCLUDE_DIRS=${messages_include_dirs}"
        -D "GTEST_LIBRARY=$<TARGET_FILE:GTest::gtest>"
        -D "GTEST_INCLUDE_DIRS=${gtest_include_dirs}"
        -D "JSON_INCLUDE_DIRS=${json_include_dirs}"
        -D "JSON_SCHEMA_VALIDATOR_LIBRARY=$<TARGET_FILE:nlohmann_json_schema_validator::validator>"
        -D "JSON_SCHEMA_VALIDATOR_INCLUDE_DIRS=${validator_include_dirs}")

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