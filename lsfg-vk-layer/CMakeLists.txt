set(LAYER_SOURCES
    "src/entrypoint.cpp"
    "src/instance.cpp"
    "src/swapchain.cpp")

add_library(lsfg-vk-layer SHARED ${LAYER_SOURCES})

target_link_libraries(lsfg-vk-layer
    PUBLIC lsfg-vk-common
    PUBLIC lsfg-vk-backend)

set_target_properties(lsfg-vk-layer PROPERTIES
    CXX_VISIBILITY_PRESET hidden)

set(LSFGVK_LAYER_LIBRARY_PATH
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/liblsfg-vk-layer.so"
    CACHE STRING "Path to Vulkan layer shared object")
configure_file(
    "VkLayer_LSFGVK_frame_generation.json.in"
    "VkLayer_LSFGVK_frame_generation.json"
    @ONLY)

install(TARGETS lsfg-vk-layer
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VkLayer_LSFGVK_frame_generation.json"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/vulkan/implicit_layer.d")
