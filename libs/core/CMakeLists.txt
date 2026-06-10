add_library(core STATIC
        utils.cpp
        utils.hpp
        chat_message.cpp
        chat_message.hpp
)

target_link_libraries(core PUBLIC
        ggml::ggml
        llama
        whisper
)

get_filename_component(CORE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

target_include_directories(core PUBLIC
        $<BUILD_INTERFACE:${CORE_INCLUDE_DIR}>
)
