#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan command buffer
    class CommandBuffer {
    public:
        /// create a command buffer
        /// @param vk the vulkan instance
        /// @throws ls::vulkan_error on failure
        CommandBuffer(const vk::Vulkan& vk);

        /// submit the command buffer
        /// @throws ls::vulkan_error on failure
        void submit(); // FIXME: method needs to actually submit, depending on needs
    private:
        ls::owned_ptr<VkCommandBuffer> commandBuffer;
    };
}
