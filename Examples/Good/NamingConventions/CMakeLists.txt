add_executable(NamingConventionsTest)
zucchinify(NamingConventionsTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE NamingConventionsDemo
    STYLESHEET "${CMAKE_CURRENT_SOURCE_DIR}/features/NamingConventions.stylesheet.yaml"
)
