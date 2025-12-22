cmake_minimum_required(VERSION 3.10)
project(lsfg-vk LANGUAGES CXX)

# === user facing options
option(LSFGVK_BUILD_VULKAN_LAYER
    "Build the Vulkan layer" ON)
option(LSFGVK_BUILD_USER_INTERFACE
    "Build the user interface" ON)
option(LSFGVK_BUILD_DEBUG_TOOL
    "Build the debug tool for testing and debugging" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(WARNING "Debug builds should use Clang for better diagnostics")
    else()
        message(STATUS "Building with further diagnostics")

        set(CMAKE_CXX_CLANG_TIDY clang-tidy)

        add_compile_options(
            -Weverything
            # disable incompatible warnings
            -Wno-pre-c++20-compat-pedantic
            -Wno-c++98-compat-pedantic
            -Wno-switch-default
            -Wno-switch-enum
            # disable noisy warnings
            -Wno-missing-designated-field-initializers
            -Wno-cast-function-type-strict
            -Wno-padded
            -Wno-shadow
        )
    endif()
endif()

if(LSFGVK_BUILD_DEBUG_TOOL)
    add_compile_definitions(LSFGVK__RENDERDOC_INTEGRATION)
endif()

add_subdirectory(lsfg-vk-common)
add_subdirectory(lsfg-vk-backend)
if(LSFGVK_BUILD_VULKAN_LAYER)
    add_subdirectory(lsfg-vk-layer)
endif()
if(LSFGVK_BUILD_USER_INTERFACE)
    add_subdirectory(lsfg-vk-ui)
endif()
if(LSFGVK_BUILD_DEBUG_TOOL)
    add_subdirectory(lsfg-vk-debug)
endif()
