/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "swapchain.hpp"
#include "lsfg-vk/lsfgvk.hpp"
#include "lsfg-vk-common/configuration/config.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk;
using namespace lsfgvk::layer;

namespace {
    /// Barrier helper
    VkImageMemoryBarrier barrierHelper(VkImage handle,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkImageLayout oldLayout,
            VkImageLayout newLayout) {
        return VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = srcAccessMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = handle,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
    }
}

void layer::context_ModifySwapchainCreateInfo(const ls::GameConf& profile, uint32_t maxImages,
        VkSwapchainCreateInfoKHR& createInfo) {
    createInfo.imageUsage |=
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    switch (profile.pacing) {
        case ls::Pacing::None:
            createInfo.minImageCount += profile.multiplier;
            if (maxImages && createInfo.minImageCount > maxImages)
                createInfo.minImageCount = maxImages;

            createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            break;
    }
}

Swapchain::Swapchain(const vk::Vulkan& vk, lsfgvk::Instance& backend,
            ls::GameConf profile, SwapchainInfo info) :
        instance(backend),
        profile(std::move(profile)), info(std::move(info)) {
    const VkExtent2D extent = this->info.extent;

    try {
        this->ctx = std::make_unique<lsfgvk::Context>(
            backend,
            extent.width, extent.height,
            this->profile.flow_scale,
            this->profile.performance_mode
        );
        this->total = static_cast<uint32_t>(this->profile.multiplier) - 1;
    } catch (const std::exception& e) {
        throw ls::error("Failed to create swapchain context", e);
    }

    const auto exportedFds = this->ctx->exportFds();
    this->sourceImage.emplace(vk,
        extent, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        exportedFds.sourceFd, std::nullopt, 2);
    this->destinationImage.emplace(vk,
        extent, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        exportedFds.destinationFd);
    this->syncSemaphore.emplace(vk, 0, exportedFds.syncFd);

    this->renderCommandBuffer.emplace(vk);
    this->renderFence.emplace(vk);
    this->finalSemaphore.emplace(vk);
    for (size_t i = 0; i < this->total; i++) {
        this->passes.emplace_back(RenderPass {
            .commandBuffer = vk::CommandBuffer(vk),
            .acquireSemaphore = vk::Semaphore(vk),
            .copySemaphore = vk::Semaphore(vk)
        });
    }
}

VkResult Swapchain::present(const vk::Vulkan& vk,
        VkQueue queue, VkSwapchainKHR swapchain,
        void* next_chain, uint32_t imageIdx,
        const std::vector<VkSemaphore>& semaphores) {
    const auto& swapchainImage = this->info.images.at(imageIdx);
    const auto sourceImageIdx{static_cast<uint32_t>(this->iteration) % 2};

    // Schedule frame generation
    try {
        this->ctx->dispatch(this->total);
    } catch (const std::exception& e) {
        throw ls::error("Failed to schedule frames", e);
    }

    // Update present mode when not using pacing
    if (this->profile.pacing == ls::Pacing::None) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        auto* info = reinterpret_cast<VkSwapchainPresentModeInfoEXT*>(next_chain);
        while (info) {
            if (info->sType == VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT) {
                for (size_t i = 0; i < info->swapchainCount; i++)
                    const_cast<VkPresentModeKHR*>(info->pPresentModes)[i] =
                        VK_PRESENT_MODE_FIFO_KHR;
            }

            info = reinterpret_cast<VkSwapchainPresentModeInfoEXT*>(const_cast<void*>(info->pNext));
        }
#pragma clang diagnostic pop
    }

    // Wait for completion of previous frame
    if (this->iteration && !this->renderFence->wait(vk, 150ULL * 1000 * 1000))
        throw ls::vulkan_error(VK_TIMEOUT, "vkWaitForFences() failed");
    this->renderFence->reset(vk);

    // Copy swapchain image into backend source image
    const auto& cmdbuf = *this->renderCommandBuffer;
    cmdbuf.begin(vk);

    cmdbuf.blitImage(vk,
        {
            barrierHelper(swapchainImage,
                VK_ACCESS_NONE,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            ),
            barrierHelper(this->sourceImage->handle(),
                VK_ACCESS_NONE,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            ),
        },
        { swapchainImage, this->sourceImage->handle() },
        this->sourceImage->getExtent(),
        {
            barrierHelper(swapchainImage,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            ),
        },
        0, sourceImageIdx
    );

    cmdbuf.end(vk);

    cmdbuf.submit(vk,
        semaphores, VK_NULL_HANDLE, 0,
        {}, this->syncSemaphore->handle(), this->syncValue
    );

    for (size_t i = 0; i < this->passes.size(); i++) {
        auto& pass{this->passes.at(i)};
        const bool last{i == (this->passes.size() - 1)};

        // Acquire swapchain image
        uint32_t swapchainImageIdx{};
        auto res = vk.df().AcquireNextImageKHR(vk.dev(), swapchain,
            UINT64_MAX, pass.acquireSemaphore.handle(),
            VK_NULL_HANDLE,
            &swapchainImageIdx
        );
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw ls::vulkan_error(res, "vkAcquireNextImageKHR() failed");

        const auto& aquiredSwapchainImage = this->info.images.at(swapchainImageIdx);

        // Copy backend destination image into swapchain image
        auto& cmdbuf = pass.commandBuffer;
        cmdbuf.begin(vk);

        cmdbuf.blitImage(vk,
            {
                barrierHelper(this->destinationImage->handle(),
                    VK_ACCESS_NONE,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                ),
                barrierHelper(aquiredSwapchainImage,
                    VK_ACCESS_NONE,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                ),
            },
            { this->destinationImage->handle(), aquiredSwapchainImage },
            this->destinationImage->getExtent(),
            {
                barrierHelper(aquiredSwapchainImage,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_MEMORY_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                ),
            }
        );

        cmdbuf.end(vk);

        std::vector<VkSemaphore> signalSemaphores{ pass.copySemaphore.handle() };
        if (last)
            signalSemaphores.push_back(this->finalSemaphore->handle());

        this->syncValue++;

        cmdbuf.submit(vk,
            { pass.acquireSemaphore.handle() }, this->syncSemaphore->handle(), this->syncValue,
            signalSemaphores, last ? nullptr : this->syncSemaphore->handle(), this->syncValue + 1,
            last ? this->renderFence->handle() : VK_NULL_HANDLE
        );

        this->syncValue++;

        // Present swapchain image
        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = i ? nullptr : next_chain,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &pass.copySemaphore.handle(),
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &swapchainImageIdx,
        };
        res = vk.df().QueuePresentKHR(queue,
            &presentInfo);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw ls::vulkan_error(res, "vkQueuePresentKHR() failed");
    }

    // Present original swapchain image
    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &this->finalSemaphore->handle(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIdx,
    };
    auto res = vk.df().QueuePresentKHR(queue, &presentInfo);
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        throw ls::vulkan_error(res, "vkQueuePresentKHR() failed");

    this->iteration++;

    return res;
}
