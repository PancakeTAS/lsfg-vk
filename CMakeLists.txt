cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_SKIP_RPATH ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions("-DLSFG_NO_DEBUG")
endif()

# subprojects
add_compile_options(-fPIC
    -Wno-deprecated-declarations
    -Wno-unused-template)

add_subdirectory(thirdparty/dxbc EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/pe-parse/pe-parser-library EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/toml11 EXCLUDE_FROM_ALL)
add_subdirectory(framegen)


# main project
project(lsfg-vk
    VERSION 0.0.1
    DESCRIPTION "LSFG on Linux"
    LANGUAGES CXX)

file(GLOB SOURCES
    "src/config/*.cpp"
    "src/extract/*.cpp"
    "src/mini/*.cpp"
    "src/utils/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk SHARED ${SOURCES})

# target
set_target_properties(lsfg-vk PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
target_include_directories(lsfg-vk
    PRIVATE include)
target_link_libraries(lsfg-vk PRIVATE
    lsfg-vk-framegen pe-parse dxbc toml11 vulkan)

get_target_property(TOML11_INCLUDE_DIRS toml11 INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(lsfg-vk SYSTEM PRIVATE ${TOML11_INCLUDE_DIRS})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(lsfg-vk PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# diagnostics
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(lsfg-vk PROPERTIES
        EXPORT_COMPILE_COMMANDS ON)
endif()

if(LSFGVK_EXCESS_DEBUG)
    message(STATUS "LSFGVK_EXCESS_DEBUG is only compatible with clang")
    target_compile_options(lsfg-vk PRIVATE
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

    set_target_properties(lsfg-vk PROPERTIES
        CXX_CLANG_TIDY clang-tidy)
endif()

# install
install(FILES "${CMAKE_BINARY_DIR}/liblsfg-vk.so"
    DESTINATION lib)
install(FILES "${CMAKE_SOURCE_DIR}/VkLayer_LS_frame_generation.json"
    DESTINATION share/vulkan/implicit_layer.d)

set(conf_file "$ENV{HOME}/.config/lsfg-vk/conf.toml")
if(NOT EXISTS "${conf_file}")
    install(FILES "${CMAKE_SOURCE_DIR}/conf.toml"
        DESTINATION ~/.config/lsfg-vk)
endif()
