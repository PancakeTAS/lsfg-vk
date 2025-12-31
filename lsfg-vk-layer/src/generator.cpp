/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "generator.hpp"
#include "hooks/device.hpp"
#include "hooks/layer.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/configuration/config.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"

#include <cstdint>
#include <exception>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk;
using namespace lsfgvk::layer;

Generator::Generator(MyVkLayer& layer, MyVkDevice& device,
            VkExtent2D extent, VkFormat format) :
        instance(std::ref(layer.backend())), vk(std::ref(device.vkd())) {
    const auto& profile = layer.profile();
    const bool hdr = format > 57;

    // create shared objects
    std::vector<int> sourceFds(2);
    std::vector<int> destinationFds(profile.multiplier - 1);

    this->sourceImages.reserve(sourceFds.size());
    for (int& fd : sourceFds)
        this->sourceImages.emplace_back(vk,
            extent, hdr ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            std::nullopt, &fd);

    this->destinationImages.reserve(destinationFds.size());
    for (int& fd : destinationFds)
        this->destinationImages.emplace_back(vk,
            extent, hdr ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            std::nullopt, &fd);

    int syncFd{};
    this->syncSemaphore.emplace(vk, 0, std::nullopt, &syncFd);

    // create backend context
    try {
        this->ctx = ls::owned_ptr<ls::R<backend::Context>>(
            new ls::R<backend::Context>(instance.get().openContext(
                { sourceFds.at(0), sourceFds.at(1) }, destinationFds, syncFd,
                extent.width, extent.height,
                hdr, 1.0F / profile.flow_scale, profile.performance_mode
            )),
            [instance = &instance.get()](ls::R<backend::Context>& ctx) {
                instance->closeContext(ctx);
            }
        );

        backend::makeLeaking(); // don't worry about it :3
    } catch (const std::exception& e) {
        throw ls::error("failed to create frame generation context", e);
    }
}

#define FILL_BARRIER(handle) \
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, \
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, \
    .image = (handle), \
    .subresourceRange = { \
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, \
        .levelCount = 1, \
        .layerCount = 1 \
    }

std::pair<VkSemaphore, uint64_t> Generator::prepare(vk::CommandBuffer& cmdbuf,
        VkImage swapchainImage) {
    const auto& sourceImage = this->sourceImages.at(this->frameIdx % 2);

    cmdbuf.blitImage(this->vk,
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                FILL_BARRIER(swapchainImage)
            },
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                FILL_BARRIER(sourceImage.handle())
            },
        },
        { swapchainImage, sourceImage.handle() },
        sourceImage.getExtent(),
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                FILL_BARRIER(swapchainImage)
            }
        }
    );

    return { this->syncSemaphore->handle(), this->syncValue++ };
}

void Generator::schedule() {
    try {
        this->instance.get().scheduleFrames(this->ctx.get());
    } catch (const std::exception& e) {
        throw ls::error("failed to schedule frames", e);
    }

    this->frameIdx++;
}

std::pair<VkSemaphore, uint64_t> Generator::obtain(vk::CommandBuffer& cmdbuf,
        VkImage swapchainImage) {
    const auto& destinationImage = this->destinationImages.at(this->generatedIdx++ % this->destinationImages.size());

    cmdbuf.blitImage(this->vk,
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                FILL_BARRIER(destinationImage.handle())
            },
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                FILL_BARRIER(swapchainImage)
            },
        },
        { destinationImage.handle(), swapchainImage },
        destinationImage.getExtent(),
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                FILL_BARRIER(swapchainImage)
            }
        }
    );

    return { this->syncSemaphore->handle(), this->syncValue++ };
}
