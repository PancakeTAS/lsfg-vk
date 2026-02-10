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
        /// @param importFd optional file descriptor to import from
        /// @param markExport whether to mark the semaphore as exportable
        /// @throws ls::vulkan_error on failure
        Semaphore(const vk::Vulkan& vk,
            std::optional<int> importFd = std::nullopt,
            bool markExport = false);

        /// export the semaphore to a file descriptor
        /// @param vk the vulkan instance
        /// @return the exported file descriptor
        /// @throws ls::vulkan_error on failure
        [[nodiscard]] int exportToFd(const vk::Vulkan& vk) const;

        /// get the underlying VkSemaphore handle
        /// @return the VkSemaphore handle
        [[nodiscard]] const auto& handle() const { return *this->semaphore; }
    private:
        ls::owned_ptr<VkSemaphore> semaphore;
    };
}
