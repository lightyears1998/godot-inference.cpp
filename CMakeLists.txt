cmake_minimum_required(VERSION 3.31)

# Based on godot-cpp-template
# https://github.com/godotengine/godot-cpp-template/blob/c5d45ad8abe3f7878bf3fd6b6508a6f1702652cc/CMakeLists.txt

project(godot-llama-cpp
        VERSION 1.0
        DESCRIPTION "This repository serves as a quickstart template for GDExtension development with Godot 4.0+."
        HOMEPAGE_URL "https://github.com/enetheru/godot-cpp-template/tree/main"
        LANGUAGES CXX
)

# Suppress warning
if (CMAKE_C_COMPILER)
endif ()

# Cpp standards
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure MSVC
if (WIN32 AND MSVC)
    add_compile_options("/utf-8")

    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(UNICODE)
    add_compile_definitions(VC_EXTRALEAN)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(NOGDI)

    # Dynamic-linked CRT
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    # or Static-linked CRT
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(LIBNAME "godot_inference_cpp")
set(GODOT_PROJECT_DIR "project" CACHE STRING "The directory of a Godot project folder")

if (NOT DEFINED GODOTCPP_TARGET)
    set(GODOTCPP_TARGET $<IF:$<CONFIG:Debug>,template_debug,template_release>)
endif ()

# project dependencies
find_package(Boost CONFIG REQUIRED COMPONENTS assert)
find_path(BEXT_SML_INCLUDE_DIRS "boost/sml.hpp")
find_package(fmt CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(ggml CONFIG REQUIRED)
find_package(llama CONFIG REQUIRED)
find_package(whisper CONFIG REQUIRED)
find_package(simdutf CONFIG REQUIRED)

# godot-cpp dependencies
find_package(Python3 3.4 REQUIRED)
find_program(GIT git REQUIRED)

# godot-cpp boostrap
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/godot-cpp/src")
    message(NOTICE "godot-cpp bindings source not found")
    message(NOTICE "initializing/updating the godot-cpp submodule...")

    # update the c++ bindings submodule to populate it with the necessary source for the library
    execute_process(
        COMMAND git submodule update --init godot-cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()
set(GODOTCPP_IS_TOP_LEVEL ON)
add_subdirectory(godot-cpp SYSTEM)

# Add godot-cpp's module path and include the exported functions.
# This is made available for documentation generation
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${godot-cpp_SOURCE_DIR}/cmake")
include(GodotCPPModule)

# The godot-cpp target has some of useful properties attached that can be retrieved like so.
get_target_property(GODOTCPP_SUFFIX godot::cpp GODOTCPP_SUFFIX)
get_target_property(GODOTCPP_PLATFORM godot::cpp GODOTCPP_PLATFORM)

set(GODOT_PROJECT_BINARY_DIR "${PROJECT_SOURCE_DIR}/${GODOT_PROJECT_DIR}/bin/${GODOTCPP_PLATFORM}")
file(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/doc_classes/*.xml")

add_subdirectory(libs)
add_subdirectory(src)

add_custom_target(package_7z
        COMMAND 7z a -mx=9
        "${CMAKE_CURRENT_LIST_DIR}/bin/release.7z"
        "${CMAKE_CURRENT_LIST_DIR}/project/bin"
        COMMENT "Pack resources for release"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
)
