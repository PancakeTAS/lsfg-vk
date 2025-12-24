cmake_minimum_required(VERSION 3.10)
project(lsfg-vk LANGUAGES CXX)
include(GNUInstallDirs)

# === READ HERE FOR BUILD OPTIONS ===
option(LSFGVK_BUILD_VK_LAYER     "Build the Vulkan layer"            ON)
option(LSFGVK_BUILD_UI           "Build the user interface"         OFF)
option(LSFGVK_BUILD_CLI          "Build the command line interface"  ON)
option(LSFGVK_INSTALL_DEVELOP    "Install development libraries and headers"      OFF)
option(LSFGVK_INSTALL_XDG_FILES  "Install the application icon and desktop files" OFF)
option(LSFGVK_LAYER_LIBRARY_PATH "Change where Vulkan searches for the layer library" liblsfg-vk-layer.so)
option(LSFGVK_TESTING_RENDERDOC  "Enable RenderDoc integration for testing purposes" OFF)
# === READ HERE FOR BUILD OPTIONS ===

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SKIP_RPATH ON)

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

if(LSFGVK_TESTING_RENDERDOC)
    add_compile_definitions(LSFGVK_TESTING_RENDERDOC)
endif()

add_subdirectory(lsfg-vk-common)
add_subdirectory(lsfg-vk-backend)
if(LSFGVK_BUILD_VK_LAYER)
    add_subdirectory(lsfg-vk-layer)
endif()
if(LSFGVK_BUILD_UI)
    add_subdirectory(lsfg-vk-ui)
endif()
if(LSFGVK_BUILD_CLI)
    add_subdirectory(lsfg-vk-cli)
endif()
