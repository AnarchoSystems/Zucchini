add_subdirectory(Calculator)
add_subdirectory(Tables)
add_subdirectory(DocStrings)
add_subdirectory(Notes)

add_test(NAME CMakeIntegration.RebuildsAfterFeatureChanges
	COMMAND "${CMAKE_COMMAND}"
		-D "TEMPLATE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/IncrementalCMake"
		-D "TEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/IncrementalCMake"
		-D "ZUCCHINIFY_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/IncrementalCMake/AssertIncrementalRebuild.cmake")
