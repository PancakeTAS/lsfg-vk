cmake_minimum_required(VERSION 3.29)

# cmake options
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_RPATH ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# subprojects
include(cmake/FetchDXVK.cmake)
include(cmake/FetchPeParse.cmake)

add_subdirectory(lsfg-vk-gen)

# main project
project(lsfg-vk
    VERSION 0.0.1
    DESCRIPTION "lsfg-vk: LSFG on Linux through Vulkan"
    LANGUAGES CXX)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Debug Vulkan configuration
message(STATUS "Vulkan_FOUND: ${Vulkan_FOUND}")
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}")
message(STATUS "Vulkan_VERSION: ${Vulkan_VERSION}")

add_subdirectory(lsfg-vk-gen)


file(GLOB SOURCES
    "src/loader/*.cpp"
    "src/mini/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk SHARED ${SOURCES})

target_include_directories(lsfg-vk
    PRIVATE include)
target_link_libraries(lsfg-vk
    PRIVATE lsfg-vk-gen Vulkan::Vulkan)
set_target_properties(lsfg-vk
    PROPERTIES CXX_CLANG_TIDY clang-tidy)

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
    # required for vulkan
    -Wno-cast-function-type-strict
)

install(FILES "${CMAKE_BINARY_DIR}/liblsfg-vk.so" DESTINATION lib)
