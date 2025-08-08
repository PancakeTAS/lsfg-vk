cmake_minimum_required(VERSION 3.10)

if(NOT LSFGVK_EXCESS_DEBUG)
    set(CMAKE_C_VISIBILITY_PRESET "hidden")
    set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
endif()

project(lsfg-vk-test
    DESCRIPTION "Test: lsfg-vk"
    LANGUAGES CXX)

file(GLOB SOURCES
    "src/*.cpp"
)

add_executable(lsfg-vk-test ${SOURCES})

# target
set_target_properties(lsfg-vk-test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
target_include_directories(lsfg-vk-test
    PRIVATE include)
target_link_libraries(lsfg-vk-test PUBLIC
    vulkan lsfg-vk)

# diagnostics
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(lsfg-vk-test PROPERTIES
        EXPORT_COMPILE_COMMANDS ON)
endif()

if(LSFGVK_EXCESS_DEBUG)
    target_compile_options(lsfg-vk-test PRIVATE
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

    set_target_properties(lsfg-vk-test PROPERTIES
        CXX_CLANG_TIDY clang-tidy)
endif()
