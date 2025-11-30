#pragma once

#include "../helpers/pointers.hpp"
#include "descriptor_set.hpp"
#include "shader.hpp"
#include "vulkan.hpp"

#include <cstdint>
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
        /// @throws ls::vulkan_error on failure
        void submit(); // FIXME: method needs to actually submit, depending on needs
    private:
        ls::owned_ptr<VkCommandBuffer> commandBuffer;
    };
}
