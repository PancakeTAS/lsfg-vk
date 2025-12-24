/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <vulkan/vulkan_core.h>

namespace vk {
    /// limits for descriptor pools
    struct Limits {
        uint32_t sets;
        uint32_t uniform_buffers;
        uint32_t samplers;
        uint32_t sampled_images;
        uint32_t storage_images;
    };

    /// vulkan descriptor pool
    class DescriptorPool {
    public:
        /// create a descriptor pool
        /// @param vk the vulkan instance
        /// @param limits the limits for the descriptor pool
        /// @throws ls::vulkan_error on failure
        DescriptorPool(const vk::Vulkan& vk, const Limits& limits);

        /// get the underlying handle
        /// @return the underlying handle
        [[nodiscard]] const auto& handle() const { return *this->descriptor_pool; }
    private:
        ls::owned_ptr<VkDescriptorPool> descriptor_pool;
    };
}
