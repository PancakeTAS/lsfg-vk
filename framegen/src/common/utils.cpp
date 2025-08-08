#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "common/utils.hpp"
#include "core/image.hpp"
#include "core/device.hpp"
#include "core/commandpool.hpp"
#include "core/fence.hpp"
#include "common/exception.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace LSFG;
using namespace LSFG::Utils;

BarrierBuilder& BarrierBuilder::addR2W(Core::Image& image) {
    this->barriers.emplace_back(VkImageMemoryBarrier2 {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .oldLayout = image.getLayout(),
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = image.handle(),
        .subresourceRange = {
            .aspectMask = image.getAspectFlags(),
            .levelCount = 1,
            .layerCount = 1
        }
    });
    image.setLayout(VK_IMAGE_LAYOUT_GENERAL);

    return *this;
}

BarrierBuilder& BarrierBuilder::addW2R(Core::Image& image) {
    this->barriers.emplace_back(VkImageMemoryBarrier2 {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
        .oldLayout = image.getLayout(),
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = image.handle(),
        .subresourceRange = {
            .aspectMask = image.getAspectFlags(),
            .levelCount = 1,
            .layerCount = 1
        }
    });
    image.setLayout(VK_IMAGE_LAYOUT_GENERAL);

    return *this;
}

void BarrierBuilder::build() const {
    const VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = static_cast<uint32_t>(this->barriers.size()),
        .pImageMemoryBarriers = this->barriers.data()
    };
    vkCmdPipelineBarrier2(this->commandBuffer->handle(), &dependencyInfo);
}

void Utils::clearImage(const Core::Device& device, Core::Image& image, bool white) {
    Core::Fence fence(device);
    const Core::CommandPool cmdPool(device);
    Core::CommandBuffer cmdBuf(device, cmdPool);
    cmdBuf.begin();

    const VkImageMemoryBarrier2 barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = image.getLayout(),
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = image.handle(),
        .subresourceRange = {
            .aspectMask = image.getAspectFlags(),
            .levelCount = 1,
            .layerCount = 1
        }
    };
    const VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    image.setLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdPipelineBarrier2(cmdBuf.handle(), &dependencyInfo);

    const float clearValue = white ? 1.0F : 0.0F;
    const VkClearColorValue clearColor = {{ clearValue, clearValue, clearValue, clearValue }};
    const VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1
    };
    vkCmdClearColorImage(cmdBuf.handle(),
        image.handle(), image.getLayout(),
        &clearColor,
        1, &subresourceRange);

    cmdBuf.end();

    cmdBuf.submit(device.getComputeQueue(), fence);
    if (!fence.wait(device))
        throw LSFG::vulkan_error(VK_TIMEOUT, "Failed to wait for clearing fence.");
}
