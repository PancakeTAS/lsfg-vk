#pragma once

#include "../helpers/pointers.hpp"
#include "descriptor_set.hpp"
#include "image.hpp"
#include "shader.hpp"
#include "timeline_semaphore.hpp"
#include "vulkan.hpp"

#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vk {

    using Barrier = VkImageMemoryBarrier;

    /// vulkan command buffer
    class CommandBuffer {
    public:
        /// create a command buffer
        /// @param vk the vulkan instance
        /// @throws ls::vulkan_error on failure
        CommandBuffer(const vk::Vulkan& vk);

        /// initialize an image
        /// @param image the image to initialize
        /// @param clearColor optional clear color
        void prepareImage(const vk::Image& image,
            const std::optional<VkClearColorValue>& clearColor = std::nullopt) const;

        /// dispatch a compute shader
        /// @param shader the compute shader
        /// @param set the descriptor set
        /// @param barriers image memory barriers to apply
        /// @param x dispatch size in X
        /// @param y dispatch size in Y
        /// @param z dispatch size in Z
        void dispatch(const vk::Shader& shader, const vk::DescriptorSet& set,
            const std::vector<vk::Barrier>& barriers,
                uint32_t x, uint32_t y, uint32_t z) const;

        /// submit the command buffer
        /// @param vk the vulkan instance
        /// @param waitSemaphore the semaphore to wait on
        /// @param waitValue the value to wait for
        /// @param signalSemaphore the semaphore to signal
        /// @param signalValue the value to signal
        /// @throws ls::vulkan_error on failure
        void submit(const vk::Vulkan& vk,
            const vk::TimelineSemaphore& waitSemaphore, uint64_t waitValue,
            const vk::TimelineSemaphore& signalSemaphore, uint64_t signalValue) const;

        /// submit the command buffer instantly
        /// @param vk the vulkan instance
        /// @throws ls::vulkan_error on failure
        void submit(const vk::Vulkan& vk) const;
    private:
        ls::owned_ptr<VkCommandBuffer> commandBuffer;
    };
}
