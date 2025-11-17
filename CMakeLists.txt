cmake_minimum_required(VERSION 3.10)
project(lsfg-vk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
            -Wno-c++98-compat
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

add_subdirectory(lsfg-vk-common)
add_subdirectory(lsfg-vk-backend)
