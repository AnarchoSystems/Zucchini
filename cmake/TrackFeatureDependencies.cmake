if(NOT DEFINED FEATURE_DIR OR NOT DEFINED TRIGGER OR NOT DEFINED STATE)
    message(FATAL_ERROR "TrackFeatureDependencies.cmake requires FEATURE_DIR, TRIGGER and STATE")
endif()

file(GLOB_RECURSE features LIST_DIRECTORIES false "\${FEATURE_DIR}/*.feature")
list(SORT features)

set(current "")
foreach(feature IN LISTS features)
    file(SHA256 "\${feature}" hash)
    string(APPEND current "\${feature}=\${hash}\n")
endforeach()

if(EXISTS "\${STATE}")
    file(READ "\${STATE}" previous)
else()
    set(previous "")
endif()

if(NOT current STREQUAL previous)
    file(WRITE "\${STATE}" "\${current}")
    file(TOUCH "\${TRIGGER}")
endif()
