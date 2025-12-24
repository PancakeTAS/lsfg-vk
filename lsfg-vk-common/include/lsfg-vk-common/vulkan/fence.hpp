/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <cstdint>

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan fence
    class Fence {
    public:
        /// create a fence
        /// @param vk the vulkan instance
        /// @throws ls::vulkan_error on failure
        Fence(const vk::Vulkan& vk);

        /// reset the fence
        /// @param vk the vulkan instance
        /// @throws ls::vulkan_error on failure
        void reset(const vk::Vulkan& vk) const;

        /// wait for the fence
        /// @param vk the vulkan instance
        /// @param timeout the timeout in nanoseconds, or UINT64_MAX for no timeout
        /// @returns true if the fence signaled, false if it timed out
        /// @throws ls::vulkan_error on failure
        [[nodiscard]] bool wait(const vk::Vulkan& vk, uint64_t timeout = UINT64_MAX) const;

        /// get the fence handle
        /// @return the fence handle
        [[nodiscard]] VkFence handle() const { return *this->fence; }
    private:
        ls::owned_ptr<VkFence> fence;
    };
}
