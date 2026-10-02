add_subdirectory(Calculator)
add_subdirectory(Tables)
add_subdirectory(DocStrings)
add_subdirectory(Notes)
add_subdirectory(LegacyString)
add_subdirectory(NamingConventions)

get_target_property(pretest_gherkin_include_dirs cucumber::gherkin INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(pretest_messages_include_dirs cucumber::messages INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(pretest_json_include_dirs nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(pretest_gtest_include_dirs GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
if(CMAKE_CONFIGURATION_TYPES)
	set(pretest_build_config "$<CONFIG>")
else()
	set(pretest_build_config "")
endif()

add_test(NAME CMakeIntegration.PreTestDiscovery
	COMMAND "${CMAKE_COMMAND}"
		-D "TEST_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/PreTestDiscovery"
		-D "TEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/PreTestDiscovery"
		-D "TEST_BUILD_CONFIG=${pretest_build_config}"
		-D "CTEST_EXECUTABLE=${CMAKE_CTEST_COMMAND}"
		-D "ZUCCHINI_PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
		-D "ZUCCHINI_EXECUTABLE=$<TARGET_FILE:Zucchini>"
		-D "ZUCCHINI_LIBRARY=$<TARGET_FILE:LibZucchini>"
		-D "ZUCCHINI_RUNTIME_LIBRARY=$<TARGET_FILE:ZucchiniRuntime>"
		-D "CUCUMBER_GHERKIN_LIBRARY=$<TARGET_FILE:cucumber::gherkin>"
		-D "CUCUMBER_GHERKIN_INCLUDE_DIRS=${pretest_gherkin_include_dirs}"
		-D "CUCUMBER_MESSAGES_LIBRARY=$<TARGET_FILE:cucumber::messages>"
		-D "CUCUMBER_MESSAGES_INCLUDE_DIRS=${pretest_messages_include_dirs}"
		-D "GTEST_LIBRARY=$<TARGET_FILE:GTest::gtest>"
		-D "GTEST_INCLUDE_DIRS=${pretest_gtest_include_dirs}"
		-D "JSON_INCLUDE_DIRS=${pretest_json_include_dirs}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/PreTestDiscovery/RunPreTestDiscovery.cmake")

add_test(NAME CMakeIntegration.RebuildsAfterFeatureChanges
	COMMAND "${CMAKE_COMMAND}"
		-D "TEMPLATE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/IncrementalCMake"
		-D "TEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/IncrementalCMake"
		-D "ZUCCHINIFY_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/IncrementalCMake/AssertIncrementalRebuild.cmake")
