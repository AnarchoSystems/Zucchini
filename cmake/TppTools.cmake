include_guard(GLOBAL)

# tpp and tpp2cpp are build tools: fetched once via get-tpp.sh and exposed as
# imported executables. Generated C++ uses tpp2cpp's embedded standalone
# runtime, so lib_tpp is not a build dependency of Zucchini.
set(ZUCCHINI_TPP_VERSION "v0.15.0" CACHE STRING "Version of the tpp toolchain to use.")
set(ZUCCHINI_TPP_BIN_DIR "${CMAKE_BINARY_DIR}/tpp-bin" CACHE PATH "Where the tpp build tools are placed.")
option(ZUCCHINI_FETCH_TPP "Download the tpp build tools with get-tpp.sh." ON)

set(ZUCCHINI_TPP_EXECUTABLE "${ZUCCHINI_TPP_BIN_DIR}/tpp")
set(ZUCCHINI_TPP2CPP_EXECUTABLE "${ZUCCHINI_TPP_BIN_DIR}/tpp2cpp")

if(ZUCCHINI_FETCH_TPP)
    add_custom_command(
        OUTPUT "${ZUCCHINI_TPP_EXECUTABLE}" "${ZUCCHINI_TPP2CPP_EXECUTABLE}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${ZUCCHINI_TPP_BIN_DIR}"
        COMMAND bash "${CMAKE_SOURCE_DIR}/get-tpp.sh"
                -exact-version "${ZUCCHINI_TPP_VERSION}"
                -o "${ZUCCHINI_TPP_BIN_DIR}"
                tpp tpp2cpp
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Fetching tpp toolchain ${ZUCCHINI_TPP_VERSION}"
        VERBATIM
    )
    add_custom_target(ZucchiniTppTools DEPENDS "${ZUCCHINI_TPP_EXECUTABLE}" "${ZUCCHINI_TPP2CPP_EXECUTABLE}")
else()
    find_program(ZUCCHINI_TPP_EXECUTABLE tpp REQUIRED)
    find_program(ZUCCHINI_TPP2CPP_EXECUTABLE tpp2cpp REQUIRED)
    add_custom_target(ZucchiniTppTools)
endif()

# Compiles a tpp project and attaches the generated sources to a target.
# The generated C++ embeds its own tpp runtime, so no lib_tpp dependency is
# required by the consuming target.
function(zucchini_add_tpp target)
    cmake_parse_arguments(TPP "" "SOURCE_DIR;NAME;NAMESPACE" "EXTRA_INCLUDES" ${ARGN})

    set(generated "${CMAKE_CURRENT_BINARY_DIR}/tpp-generated")
    set(ir "${generated}/${TPP_NAME}.ir.json")
    set(runtime "${generated}/${TPP_NAME}_runtime.h")
    set(types "${generated}/${TPP_NAME}_types.h")
    set(functions "${generated}/${TPP_NAME}_functions.h")
    set(implementation "${generated}/${TPP_NAME}_implementation.cc")

    set(namespace_args)
    if(TPP_NAMESPACE)
        list(APPEND namespace_args -ns "${TPP_NAMESPACE}")
    endif()

    set(include_args)
    foreach(extra IN LISTS TPP_EXTRA_INCLUDES)
        list(APPEND include_args -i "${extra}")
    endforeach()

    file(GLOB tpp_sources CONFIGURE_DEPENDS "${TPP_SOURCE_DIR}/*.tpp" "${TPP_SOURCE_DIR}/tpp-config.json")

    add_custom_command(
        OUTPUT "${runtime}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${generated}"
        COMMAND "${CMAKE_COMMAND}"
                -DCMD="${ZUCCHINI_TPP2CPP_EXECUTABLE}"
                -DARGS=runtime
                -DOUT="${runtime}"
                -P "${CMAKE_SOURCE_DIR}/cmake/StdoutToFile.cmake"
        DEPENDS ZucchiniTppTools "${ZUCCHINI_TPP2CPP_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/cmake/StdoutToFile.cmake"
        COMMENT "tpp2cpp: embedding runtime for ${TPP_NAME}"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${ir}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${generated}"
        COMMAND "${CMAKE_COMMAND}" -E env "${ZUCCHINI_TPP_EXECUTABLE}" "${TPP_SOURCE_DIR}" > "${ir}"
        DEPENDS ZucchiniTppTools "${ZUCCHINI_TPP_EXECUTABLE}" ${tpp_sources}
        COMMENT "tpp: compiling ${TPP_NAME}"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${types}"
        COMMAND "${ZUCCHINI_TPP2CPP_EXECUTABLE}" types --input "${ir}" ${namespace_args} > "${types}"
        DEPENDS "${ir}" "${ZUCCHINI_TPP2CPP_EXECUTABLE}"
        COMMENT "tpp2cpp: ${TPP_NAME}_types.h"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${functions}"
        COMMAND "${ZUCCHINI_TPP2CPP_EXECUTABLE}" functions --standalone -i "${runtime}" ${namespace_args} -i "${types}" ${include_args} --input "${ir}"
                > "${functions}"
        DEPENDS "${ir}" "${runtime}" "${types}" "${ZUCCHINI_TPP2CPP_EXECUTABLE}"
        COMMENT "tpp2cpp: ${TPP_NAME}_functions.h"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${implementation}"
        COMMAND "${ZUCCHINI_TPP2CPP_EXECUTABLE}" impl --standalone -i "${runtime}" ${namespace_args} -i "${functions}" ${include_args} --input "${ir}"
                > "${implementation}"
        DEPENDS "${ir}" "${runtime}" "${functions}" "${types}" "${ZUCCHINI_TPP2CPP_EXECUTABLE}"
        COMMENT "tpp2cpp: ${TPP_NAME}_implementation.cc"
        VERBATIM
    )

    target_sources(${target} PRIVATE "${implementation}" "${functions}" "${types}" "${runtime}")
    target_include_directories(${target} PRIVATE "${generated}")
endfunction()
