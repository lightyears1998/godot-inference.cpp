add_library(${LIBNAME} SHARED
        register_types.cpp
        register_types.h
        utils.cpp
        utils.hpp
        game_db.cpp
        game_db.hpp
        image_pack.cpp
        image_pack.hpp
)

set_property(TARGET ${LIBNAME} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${LIBNAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(${LIBNAME} PRIVATE
        godot-cpp
        fmt::fmt
)

set_target_properties(${LIBNAME}
        PROPERTIES
        # The generator expression here prevents msvc from adding a Debug or Release subdir.
        RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/bin/${GODOTCPP_PLATFORM}>"

        PREFIX ""
        OUTPUT_NAME "${LIBNAME}${GODOTCPP_SUFFIX}"
)

add_custom_command(TARGET ${LIBNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${LIBNAME}>" "${GODOT_PROJECT_BINARY_DIR}/$<TARGET_FILE_NAME:${LIBNAME}>"
)

if (DOC_XML)
    if (GODOTCPP_TARGET MATCHES "editor|template_debug")
        target_doc_sources(${LIBNAME} ${DOC_XML})
    endif ()
endif ()
