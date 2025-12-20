#pragma once

#include "../helpers/pointers.hpp"
#include "buffer.hpp"
#include "descriptor_set.hpp"
#include "image.hpp"
#include "shader.hpp"
#include "vulkan.hpp"

#include <cstdint>
#include <utility>
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

        /// blit an image
        /// @param vk the vulkan instance
        /// @param preBarriers image memory barriers to apply before blit
        /// @param images source and destination images
        /// @param extent the extent of the blit
        /// @param postBarriers image memory barriers to apply after blit
        /// throws ls::vulkan_error on failure
        void blitImage(const vk::Vulkan& vk,
            const std::vector<vk::Barrier>& preBarriers,
            std::pair<VkImage, VkImage> images, VkExtent2D extent,
            const std::vector<vk::Barrier>& postBarriers) const;

        /// insert a bunch of barriers
        /// @param vk the vulkan instance
        /// @param barriers image memory barriers to apply
        /// throws ls::vulkan_error on failure
        void insertBarriers(const vk::Vulkan& vk,
            const std::vector<vk::Barrier>& barriers) const;

        /// dispatch a compute shader
        /// @param vk the vulkan instance
        /// @param shader the compute shader
        /// @param set the descriptor set
        /// @param barriers image memory barriers to apply
        /// @param x dispatch size in X
        /// @param y dispatch size in Y
        /// @param z dispatch size in Z
        void dispatch(const vk::Vulkan& vk, const vk::Shader& shader, const vk::DescriptorSet& set,
            const std::vector<vk::Barrier>& barriers,
            uint32_t x, uint32_t y, uint32_t z) const;

        /// copy buffer to image
        /// @param vk the vulkan instance
        /// @param buffer the source buffer
        /// @param image the destination image
        void copyBufferToImage(const vk::Vulkan& vk,
            const vk::Buffer& buffer, const vk::Image& image) const;

        /// submit the command buffer
        /// @param vk the vulkan instance
        /// @param waitSemaphores the semaphores to wait on
        /// @param waitTimelineSemaphore the timeline semaphore to wait on
        /// @param waitValue the value to wait for
        /// @param signalSemaphores the semaphores to signal
        /// @param signalTimelineSemaphore the timeline semaphore to signal
        /// @param signalValue the value to signal
        /// @param fence optional fence to signal on completion
        /// @throws ls::vulkan_error on failure
        void submit(const vk::Vulkan& vk,
            std::vector<VkSemaphore> waitSemaphores,
            VkSemaphore waitTimelineSemaphore, uint64_t waitValue,
            std::vector<VkSemaphore> signalSemaphores,
            VkSemaphore signalTimelineSemaphore, uint64_t signalValue,
            VkFence fence = nullptr) const;

        /// submit the command buffer instantly
        /// @param vk the vulkan instance
        /// @throws ls::vulkan_error on failure
        void submit(const vk::Vulkan& vk) const;
    private:
        ls::owned_ptr<VkCommandBuffer> commandBuffer;
    };
}
