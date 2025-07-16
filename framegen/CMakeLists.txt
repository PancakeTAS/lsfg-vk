cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(lsfg-vk-framegen
    DESCRIPTION "Lossless Scaling Frame Generation Backend"
    LANGUAGES CXX)

file(GLOB SOURCES
    "src/common/*.cpp"
    "src/config/*.cpp"
    "src/core/*.cpp"
    "src/pool/*.cpp"
    "src/*.cpp"
    "v3.1_src/core/*.cpp"
    "v3.1_src/pool/*.cpp"
    "v3.1_src/shaders/*.cpp"
    "v3.1_src/utils/*.cpp"
    "v3.1_src/*.cpp"
    "v3.1p_src/core/*.cpp"
    "v3.1p_src/pool/*.cpp"
    "v3.1p_src/shaders/*.cpp"
    "v3.1p_src/utils/*.cpp"
    "v3.1p_src/*.cpp"
)

add_library(lsfg-vk-framegen STATIC ${SOURCES})

# target
set_target_properties(lsfg-vk-framegen PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
target_include_directories(lsfg-vk-framegen
    PUBLIC include
    PUBLIC public
    PRIVATE v3.1_include
    PRIVATE v3.1p_include)
target_link_libraries(lsfg-vk-framegen
    PUBLIC vulkan)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(lsfg-vk-framegen PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# diagnostics
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(lsfg-vk-framegen PROPERTIES
        EXPORT_COMPILE_COMMANDS ON)
endif()

if(LSFGVK_EXCESS_DEBUG)
    target_compile_options(lsfg-vk-framegen PRIVATE
        -Weverything
        # disable compat c++ flags
        -Wno-pre-c++20-compat-pedantic
        -Wno-pre-c++17-compat
        -Wno-c++98-compat-pedantic
        -Wno-c++98-compat
        # disable other flags
        -Wno-missing-designated-field-initializers
        -Wno-shadow         # allow shadowing
        -Wno-switch-enum    # ignore missing cases
        -Wno-switch-default # ignore missing default
        -Wno-padded         # ignore automatic padding
        -Wno-exit-time-destructors # allow globals
        -Wno-global-constructors   # allow globals
        -Wno-cast-function-type-strict # for vulkan
    )

    set_target_properties(lsfg-vk-framegen PROPERTIES
        CXX_CLANG_TIDY clang-tidy)
endif()
