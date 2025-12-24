/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <cstddef>

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan buffer
    class Buffer {
    public:
        /// create a buffer
        /// @param vk the vulkan instance
        /// @param data initial data uploaded to the buffer
        /// @param usage usage flags for the buffer
        /// @throws ls::vulkan_error on failure
        template<typename T>
        Buffer(const vk::Vulkan& vk, const T& data,
                VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            : Buffer(vk, reinterpret_cast<const void*>(&data), sizeof(T), usage) {}

        /// create a buffer
        /// @param vk the vulkan instance
        /// @param data initial data uploaded to the buffer
        /// @param size size of the buffer in bytes
        /// @param usage usage flags for the buffer
        /// @throws ls::vulkan_error on failure
        Buffer(const vk::Vulkan& vk, const void* data, size_t size,
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        /// get the buffer handle
        /// @return the buffer handle
        [[nodiscard]] const auto& handle() const { return this->buffer.get(); }
        /// get the size of the buffer
        /// @return the size of the buffer in bytes
        [[nodiscard]] size_t length() const { return this->size; }
    private:
        ls::owned_ptr<VkBuffer> buffer;
        ls::owned_ptr<VkDeviceMemory> memory;
        size_t size;
    };
}
