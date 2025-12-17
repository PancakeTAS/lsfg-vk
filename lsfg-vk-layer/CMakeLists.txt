set(LAYER_SOURCES
    "src/configuration/config.cpp"
    "src/configuration/detection.cpp"
    "src/context/instance.cpp"
    "src/context/swapchain.cpp"
    "src/entrypoint.cpp")

add_library(lsfg-vk-layer SHARED ${LAYER_SOURCES})

target_link_libraries(lsfg-vk-layer
    PUBLIC lsfg-vk-common
    PUBLIC lsfg-vk-backend)

target_include_directories(lsfg-vk-layer SYSTEM
    PRIVATE thirdparty/include)

set_target_properties(lsfg-vk-layer PROPERTIES
    CXX_VISIBILITY_PRESET hidden)
