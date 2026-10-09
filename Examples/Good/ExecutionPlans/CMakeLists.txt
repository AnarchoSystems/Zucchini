add_executable(ExecutionPlansTest ExecutionPlans.h)
zucchinify(ExecutionPlansTest FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE ExecutionPlans)
zucchini_add_main(ExecutionPlansTest ExecutionPlans)
foreach(case IN ITEMS collisions stale filesystem writes listing dispatch)
    add_test(NAME ExecutionPlans.${case}
        COMMAND "${CMAKE_COMMAND}"
            -D "TEST_EXECUTABLE=$<TARGET_FILE:ExecutionPlansTest>"
            -D "SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -D "WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/${case}"
            -D "CASE=${case}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckPlans.cmake")
endforeach()

add_executable(ExecutionPlanStoreExample StoreExample.cpp)
target_link_libraries(ExecutionPlanStoreExample PRIVATE Zucchini::Runtime)
add_test(NAME ExecutionPlans.StorageFailures
    COMMAND ExecutionPlanStoreExample "${CMAKE_CURRENT_BINARY_DIR}/storage-failures")
