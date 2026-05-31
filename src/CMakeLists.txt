add_library(${LIBNAME} SHARED
        register_types.cpp
        register_types.hpp
        utils.cpp
        utils.hpp
        llm_engine.cpp
        llm_engine.hpp
        llm_model.cpp
        llm_model.hpp
        llm_chat.hpp
        llm_chat.cpp
        llm_chat_message.cpp
        llm_chat_message.hpp
        constants.hpp
        llm_chat_parameters.cpp
        llm_chat_parameters.hpp
        threading.cpp
        threading.hpp
        globals.hpp
        globals.cpp
        benchmark.cpp
        benchmark.hpp
)

target_link_libraries(${LIBNAME} PRIVATE
        godot-cpp
        core
        Boost::assert
        fmt::fmt
        gsl-lite::gsl-lite
        llama
        simdutf::simdutf
)

target_include_directories(${LIBNAME} PRIVATE
        $<BUILD_INTERFACE:${BEXT_SML_INCLUDE_DIRS}>
)

set_target_properties(${LIBNAME}
        PROPERTIES
        # The generator expression here prevents msvc from adding a Debug or Release subdir.
        RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/bin/${GODOTCPP_PLATFORM}>"

        PREFIX ""
        OUTPUT_NAME "${LIBNAME}${GODOTCPP_SUFFIX}"
)

add_custom_command(TARGET ${LIBNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${LIBNAME}>" "${GODOT_PROJECT_BINARY_DIR}/$<TARGET_FILE_NAME:${LIBNAME}>"
)

if (DOC_XML)
    if (GODOTCPP_TARGET MATCHES "editor|template_debug")
        target_doc_sources(${LIBNAME} ${DOC_XML})
    endif ()
endif ()

install(TARGETS ${LIBNAME}
        COMPONENT ${LIBNAME}
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        DIRECTORIES "$<TARGET_FILE_DIR:${LIBNAME}>" "$ENV{CUDA_PATH}/bin"
        DESTINATION "./"
)
