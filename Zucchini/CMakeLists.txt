add_executable(Zucchini
    Lowering.cpp
    main.cpp
)

zucchini_add_tpp(Zucchini
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tpp"
    NAME ZucchiniTemplates
    NAMESPACE nZucchiniTemplates
)

target_link_libraries(Zucchini PRIVATE Zucchini::LibZucchini)
