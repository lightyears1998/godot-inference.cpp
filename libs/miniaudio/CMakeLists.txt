add_library(miniaudio STATIC
        miniaudio.h
        miniaudio.cpp
)

get_filename_component(MINIAUDIO_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

target_include_directories(miniaudio PUBLIC
        $<BUILD_INTERFACE:${MINIAUDIO_INCLUDE_DIR}>
)
