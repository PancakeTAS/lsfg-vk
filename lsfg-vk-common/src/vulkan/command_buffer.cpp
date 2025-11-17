#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a command buffer
    ls::owned_ptr<VkCommandBuffer> createCommandBuffer(const vk::Vulkan& vk) {
        VkCommandBuffer handle{};

        const VkCommandBufferAllocateInfo commandBufferInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = vk.cmdpool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        auto res = vkAllocateCommandBuffers(vk.dev(), &commandBufferInfo, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkAllocateCommandBuffers() failed");

        return ls::owned_ptr<VkCommandBuffer>(
            new VkCommandBuffer(handle),
            [dev = vk.dev(), pool = vk.cmdpool()](VkCommandBuffer& commandBufferModule) {
                vkFreeCommandBuffers(dev, pool, 1, &commandBufferModule);
            }
        );
    }
}

CommandBuffer::CommandBuffer(const vk::Vulkan& vk)
        : commandBuffer(createCommandBuffer(vk)) {

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    auto res = vkBeginCommandBuffer(*this->commandBuffer, &beginInfo);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkBeginCommandBuffer() failed");
}

void CommandBuffer::submit() {
    auto res = vkEndCommandBuffer(*this->commandBuffer);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkEndCommandBuffer() failed");
}
