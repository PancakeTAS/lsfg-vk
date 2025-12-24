/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/vulkan/buffer.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a buffer
    ls::owned_ptr<VkBuffer> createBuffer(const vk::Vulkan& vk, size_t size,
            VkBufferUsageFlags usage) {
        VkBuffer handle{};

        const VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };
        auto res = vk.df().CreateBuffer(vk.dev(), &bufferInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateBuffer() failed");

        return ls::owned_ptr<VkBuffer>(
            new VkBuffer(handle),
            [dev = vk.dev(), defunc = vk.df().DestroyBuffer](VkBuffer& buffer) {
                defunc(dev, buffer, nullptr);
            }
        );
    }
    /// allocate memory for a buffer
    ls::owned_ptr<VkDeviceMemory> allocateMemory(const vk::Vulkan& vk, VkBuffer buffer) {
        VkDeviceMemory handle{};

        VkMemoryRequirements reqs{};
        vk.df().GetBufferMemoryRequirements(vk.dev(), buffer, &reqs);

        auto mti = vk.findMemoryTypeIndex(
            reqs.memoryTypeBits,
            true
        );
        if (!mti.has_value())
            throw ls::vulkan_error("no suitable memory type found for buffer");

        const VkMemoryAllocateInfo memoryInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = reqs.size,
            .memoryTypeIndex = *mti
        };
        auto res = vk.df().AllocateMemory(vk.dev(), &memoryInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkAllocateMemory() failed");

        res = vk.df().BindBufferMemory(vk.dev(), buffer, handle, 0);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkBindBufferMemory() failed");

        return ls::owned_ptr<VkDeviceMemory>(
            new VkDeviceMemory(handle),
            [dev = vk.dev(), defunc = vk.df().FreeMemory](VkDeviceMemory& memory) {
                defunc(dev, memory, nullptr);
            }
        );
    }
    /// copy data to a buffer
    void copyDataToBuffer(const vk::Vulkan& vk,
            VkDeviceMemory memory, const void* data, size_t size) {
        void* buf{};

        auto res = vk.df().MapMemory(vk.dev(), memory, 0, size, 0, &buf);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkMapMemory() failed");

        std::copy_n(
            reinterpret_cast<const uint8_t*>(data),
            size,
            reinterpret_cast<uint8_t*>(buf)
        );

        vk.df().UnmapMemory(vk.dev(), memory);
    }
}

Buffer::Buffer(const vk::Vulkan& vk, const void* data, size_t size, VkBufferUsageFlags usage) :
        buffer(createBuffer(vk, size, usage)),
        memory(allocateMemory(vk, *this->buffer)),
        size(size) {
    copyDataToBuffer(vk, *this->memory, data, size);
}
