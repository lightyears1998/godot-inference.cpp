add_library(core STATIC
        utils.cpp
        utils.hpp
)

target_link_libraries(core PUBLIC
        llama
)

target_include_directories(core PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
