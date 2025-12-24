/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <optional>

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan semaphore
    class Semaphore {
    public:
        /// create a semaphore
        /// @param vk the vulkan instance
        /// @param fd optional file descriptor to import the semaphore from
        /// @throws ls::vulkan_error on failure
        Semaphore(const vk::Vulkan& vk, std::optional<int> fd = std::nullopt);

        /// get the underlying VkSemaphore handle
        /// @return the VkSemaphore handle
        [[nodiscard]] const auto& handle() const { return *this->semaphore; }
    private:
        ls::owned_ptr<VkSemaphore> semaphore;
    };
}
