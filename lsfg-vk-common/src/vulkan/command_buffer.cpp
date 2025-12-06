#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/buffer.hpp"
#include "lsfg-vk-common/vulkan/descriptor_set.hpp"
#include "lsfg-vk-common/vulkan/fence.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/shader.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstdint>
#include <optional>
#include <vector>

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
        auto res = vk.df().AllocateCommandBuffers(vk.dev(), &commandBufferInfo, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkAllocateCommandBuffers() failed");

        return ls::owned_ptr<VkCommandBuffer>(
            new VkCommandBuffer(handle),
            [dev = vk.dev(), pool = vk.cmdpool(), defunc = vk.df().FreeCommandBuffers](
                VkCommandBuffer& commandBufferModule
            ) {
                defunc(dev, pool, 1, &commandBufferModule);
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
    auto res = vk.df().BeginCommandBuffer(*this->commandBuffer, &beginInfo);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkBeginCommandBuffer() failed");
}

void CommandBuffer::prepareImage(const vk::Vulkan& vk,
        const vk::Image& image,
        const std::optional<VkClearColorValue>& clearColor) const {
    const VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = image.handle(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    vk.df().CmdPipelineBarrier(*this->commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    if (clearColor.has_value()) {
        vk.df().CmdClearColorImage(*this->commandBuffer,
            image.handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            &clearColor.value(),
            1, &barrier.subresourceRange
        );
    }
}

void CommandBuffer::dispatch(const vk::Vulkan& vk,
        const vk::Shader& shader,
        const vk::DescriptorSet& set,
        const std::vector<vk::Barrier>& barriers,
        uint32_t x, uint32_t y, uint32_t z) const {
    vk.df().CmdPipelineBarrier(*this->commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        static_cast<uint32_t>(barriers.size()), barriers.data()
    );
    vk.df().CmdBindPipeline(*this->commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        shader.pipeline()
    );
    vk.df().CmdBindDescriptorSets(*this->commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        shader.pipelinelayout(),
        0, 1, &set.handle(),
        0, nullptr
    );
    vk.df().CmdDispatch(*this->commandBuffer, x, y, z);
}

void CommandBuffer::copyBufferToImage(const vk::Vulkan& vk,
        const vk::Buffer& buffer, const vk::Image& image) const {
    const VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = image.handle(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    vk.df().CmdPipelineBarrier(*this->commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    const VkBufferImageCopy region{
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1
        },
        .imageExtent = {
            .width = image.getExtent().width,
            .height = image.getExtent().height,
            .depth = 1
        }
    };
    vk.df().CmdCopyBufferToImage(*this->commandBuffer,
        buffer.handle(), image.handle(),
        VK_IMAGE_LAYOUT_GENERAL, 1, &region
    );
}


void CommandBuffer::submit(const vk::Vulkan& vk,
        const vk::TimelineSemaphore& waitSemaphore, uint64_t waitValue,
        const vk::TimelineSemaphore& signalSemaphore, uint64_t signalValue) const {
    auto res = vk.df().EndCommandBuffer(*this->commandBuffer);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkEndCommandBuffer() failed");


    const VkTimelineSemaphoreSubmitInfo timelineInfo{
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .waitSemaphoreValueCount = 1,
        .pWaitSemaphoreValues = &waitValue,
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = &signalValue
    };
    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &timelineInfo,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore.handle(),
        .pWaitDstStageMask = &stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &*this->commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &signalSemaphore.handle()
    };
    res = vk.df().QueueSubmit(vk.queue(), 1, &submitInfo, VK_NULL_HANDLE);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkQueueSubmit() failed");
}

void CommandBuffer::submit(const vk::Vulkan& vk) const {
    auto res = vk.df().EndCommandBuffer(*this->commandBuffer);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkEndCommandBuffer() failed");

    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &*this->commandBuffer
    };
    const vk::Fence fence{vk};
    res = vk.df().QueueSubmit(vk.queue(), 1, &submitInfo, fence.handle());
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkQueueSubmit() failed");

    if (!fence.wait(vk))
        throw ls::vulkan_error(VK_TIMEOUT, "Fence::wait() timed out");
}
