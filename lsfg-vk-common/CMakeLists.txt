set(COMMON_SOURCES
    "src/helpers/errors.cpp"
    "src/vulkan/buffer.cpp"
    "src/vulkan/image.cpp"
    "src/vulkan/sampler.cpp"
    "src/vulkan/fence.cpp"
    "src/vulkan/semaphore.cpp"
    "src/vulkan/timeline_semaphore.cpp"
    "src/vulkan/command_buffer.cpp"
    "src/vulkan/descriptor_set.cpp"
    "src/vulkan/shader.cpp"
    "src/vulkan/vulkan.cpp")

add_library(lsfg-vk-common STATIC ${COMMON_SOURCES})

target_include_directories(lsfg-vk-common
    PUBLIC include)
target_link_libraries(lsfg-vk-common
    vulkan)
