# Run the assertions in a child so even FATAL_ERROR returns control for restoration.
set(fixtures "${TEST_SOURCE_DIR}/features/manifest.yaml")
set(backups "${TEST_SOURCE_DIR}/before.yml.bak")
file(GLOB editing_backups "${TEST_SOURCE_DIR}/editing/*/before.yml.bak")
foreach(backup IN LISTS editing_backups)
    string(REGEX REPLACE "\\.bak$" "" fixture "${backup}")
    list(APPEND fixtures "${fixture}")
    list(APPEND backups "${backup}")
endforeach()
list(LENGTH fixtures count)
math(EXPR last "${count} - 1")
foreach(index RANGE 0 ${last})
    list(GET fixtures ${index} fixture)
    list(GET backups ${index} backup)
    file(READ "${fixture}" actual HEX)
    file(READ "${backup}" expected HEX)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "Fixture ${fixture} differs from its backup; synchronize the fixture and .bak before testing")
    endif()
endforeach()
set(arguments)
foreach(variable IN ITEMS TEST_SOURCE_DIR TEST_BINARY_DIR TEST_BUILD_CONFIG
        TEST_CXX_COMPILER TEST_CXX_FLAGS CTEST_EXECUTABLE ZUCCHINI_PROJECT_SOURCE_DIR
        BAD_EXAMPLE_SUPPORT_DIR ZUCCHINI_EXECUTABLE ZUCCHINI_LIBRARY YAML_LIBRARY
        ZUCCHINI_RUNTIME_LIBRARY CUCUMBER_GHERKIN_LIBRARY CUCUMBER_GHERKIN_INCLUDE_DIRS
        CUCUMBER_MESSAGES_LIBRARY CUCUMBER_MESSAGES_INCLUDE_DIRS GTEST_LIBRARY
        GTEST_INCLUDE_DIRS JSON_INCLUDE_DIRS)
    string(REPLACE ";" "\\;" argument "${variable}=${${variable}}")
    list(APPEND arguments -D "${argument}")
endforeach()
execute_process(COMMAND "${CMAKE_COMMAND}" ${arguments}
    -P "${CMAKE_CURRENT_LIST_DIR}/ExerciseManifestUpdates.cmake"
    RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
foreach(index RANGE 0 ${last})
    list(GET fixtures ${index} fixture)
    list(GET backups ${index} backup)
    configure_file("${backup}" "${fixture}" COPYONLY)
endforeach()
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Manifest update assertions failed (fixtures restored):\n${out}${err}")
endif()
