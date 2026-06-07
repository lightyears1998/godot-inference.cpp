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

target_include_directories(core PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/.."
)
