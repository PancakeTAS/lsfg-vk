#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/descriptor_set.hpp"
#include "lsfg-vk-common/vulkan/shader.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstdint>
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

void CommandBuffer::dispatch(const vk::Shader& shader,
        const vk::DescriptorSet& set,
        const std::vector<vk::Barrier>& barriers,
        uint32_t x, uint32_t y, uint32_t z) const {
    vkCmdPipelineBarrier(*this->commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        static_cast<uint32_t>(barriers.size()), barriers.data()
    );
    vkCmdBindPipeline(*this->commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        shader.pipeline()
    );
    vkCmdBindDescriptorSets(*this->commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        shader.pipelinelayout(),
        0, 1, &set.handle(),
        0, nullptr
    );
    vkCmdDispatch(*this->commandBuffer, x, y, z);
}

void CommandBuffer::submit() {
    auto res = vkEndCommandBuffer(*this->commandBuffer);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkEndCommandBuffer() failed");
}
