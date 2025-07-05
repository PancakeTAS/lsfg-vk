# lsfg-vk-gen subdirectory
# Note: No project() declaration needed as this is a subdirectory

file(GLOB BACKEND_SOURCES
    "src/core/*.cpp"
    "src/pool/*.cpp"
    "src/shaderchains/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk-gen SHARED ${BACKEND_SOURCES})

target_include_directories(lsfg-vk-gen
    PRIVATE include
    PUBLIC public)
target_link_libraries(lsfg-vk-gen
    PUBLIC Vulkan::Vulkan peparse dxvk)
set_target_properties(lsfg-vk-gen
    PROPERTIES CXX_CLANG_TIDY clang-tidy)

target_compile_options(lsfg-vk-gen PRIVATE
    -Weverything -fPIC
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
    -Wno-global-constructors
    # required for vulkan
    -Wno-cast-function-type-strict
    # required for peparse
    -Wno-unused-template
)
