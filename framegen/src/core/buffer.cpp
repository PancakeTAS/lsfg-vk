#include "core/buffer.hpp"
#include "core/device.hpp"
#include "common/exception.hpp"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>

using namespace LSFG::Core;

void Buffer::construct(const Core::Device& device, const void* data, VkBufferUsageFlags usage) {
    // create buffer
    const VkBufferCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = this->size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VkBuffer bufferHandle{};
    auto res = vkCreateBuffer(device.handle(), &desc, nullptr, &bufferHandle);
    if (res != VK_SUCCESS || bufferHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Failed to create Vulkan buffer");

    // find memory type
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device.getPhysicalDevice(), &memProps);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device.handle(), bufferHandle, &memReqs);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    std::optional<uint32_t> memType{};
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memReqs.memoryTypeBits & (1 << i)) && // NOLINTBEGIN
            (memProps.memoryTypes[i].propertyFlags &
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            memType.emplace(i);
            break;
        } // NOLINTEND
    }
    if (!memType.has_value())
        throw LSFG::vulkan_error(VK_ERROR_UNKNOWN, "Unable to find memory type for buffer");
#pragma clang diagnostic pop

    // allocate and bind memory
    const VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = memType.value()
    };
    VkDeviceMemory memoryHandle{};
    res = vkAllocateMemory(device.handle(), &allocInfo, nullptr, &memoryHandle);
    if (res != VK_SUCCESS || memoryHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Failed to allocate memory for Vulkan buffer");

    res = vkBindBufferMemory(device.handle(), bufferHandle, memoryHandle, 0);
    if (res != VK_SUCCESS)
        throw LSFG::vulkan_error(res, "Failed to bind memory to Vulkan buffer");

    // upload data to buffer
    uint8_t* buf{};
    res = vkMapMemory(device.handle(), memoryHandle, 0, this->size, 0, reinterpret_cast<void**>(&buf));
    if (res != VK_SUCCESS || buf == nullptr)
        throw LSFG::vulkan_error(res, "Failed to map memory for Vulkan buffer");
    std::copy_n(reinterpret_cast<const uint8_t*>(data), this->size, buf);
    vkUnmapMemory(device.handle(), memoryHandle);

    // store buffer and memory in shared ptr
    this->buffer = std::shared_ptr<VkBuffer>(
        new VkBuffer(bufferHandle),
        [dev = device.handle()](VkBuffer* img) {
            vkDestroyBuffer(dev, *img, nullptr);
        }
    );
    this->memory = std::shared_ptr<VkDeviceMemory>(
        new VkDeviceMemory(memoryHandle),
        [dev = device.handle()](VkDeviceMemory* mem) {
            vkFreeMemory(dev, *mem, nullptr);
        }
    );
}
