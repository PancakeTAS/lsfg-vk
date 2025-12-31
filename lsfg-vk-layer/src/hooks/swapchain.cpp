/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "device.hpp"
#include "instance.hpp"
#include "layer.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/semaphore.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"
#include "swapchain.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk;
using namespace lsfgvk::layer;

namespace {
    /// helper to acquire the max number of images
    uint32_t getMaxImages(const vk::Vulkan& vk, VkSurfaceKHR surface) {
        VkSurfaceCapabilitiesKHR caps{};
        auto res = vk.fi().GetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physdev(), surface, &caps);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed");

        return caps.maxImageCount;
    }
    /// helper to get swapchain images
    std::vector<VkImage> getSwapchainImages(const vk::Vulkan& vk, VkSwapchainKHR swapchain) {
        uint32_t imageCount = 0;
        auto res = vk.df().GetSwapchainImagesKHR(vk.dev(), swapchain, &imageCount, nullptr);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkGetSwapchainImagesKHR() failed to get image count");

        std::vector<VkImage> images(imageCount);
        res = vk.df().GetSwapchainImagesKHR(vk.dev(), swapchain, &imageCount, images.data());
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkGetSwapchainImagesKHR() failed to get images");

        return images;
    }
}

MyVkSwapchain::MyVkSwapchain(MyVkLayer& layer, MyVkInstance& instance, MyVkDevice& device,
            VkSwapchainCreateInfoKHR info,
            const std::function<VkSwapchainKHR(VkSwapchainCreateInfoKHR*)>& createFunc) :
        layer(std::ref(layer)), instance(std::ref(instance)), device(std::ref(device)),
        extent(info.imageExtent), format(info.imageFormat) {
    // modify create info
    const uint32_t maxImageCount = getMaxImages(this->device.get().vkd(), info.surface);
    info.minImageCount += layer.profile().multiplier;
    if (maxImageCount && info.minImageCount > maxImageCount)
        info.minImageCount = maxImageCount;

    info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // create swapchain
    this->handle = createFunc(&info);
    this->images = getSwapchainImages(this->device.get().vkd(), this->handle);

    this->reinitialize();
}

void MyVkSwapchain::reinitialize() {
    const auto& vk = this->device.get().vkd();
    this->generator.emplace(this->layer, this->device,
        this->extent,
        this->format
    );

    this->waitSemaphore.emplace(vk, 0);
    this->waitValue = 1;

    this->commandBuffer.emplace(vk);
    this->fence.emplace(vk);

    const size_t max_flight = std::max(this->images.size(), this->passes.size() + 2);

    this->passes.clear();
    for (size_t i = 0; i < max_flight; i++) {
        this->passes.emplace_back(RenderPass {
            .commandBuffer = vk::CommandBuffer(vk),
            .acquireSemaphore = vk::Semaphore(vk),
            .pcs = {
                vk::Semaphore(vk),
                vk::Semaphore(vk)
            }
        });
    }

    this->idx = 0;
}

namespace {
    /// helper to acquire the next swapchain image
    uint32_t acquireNextImageKHR(const vk::Vulkan& vk, VkSwapchainKHR swapchain,
            const vk::Semaphore& semaphore) {
        uint32_t imageIdx{};

        auto res = vk.df().AcquireNextImageKHR(vk.dev(), swapchain, UINT64_MAX,
            semaphore.handle(), VK_NULL_HANDLE, &imageIdx);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw ls::vulkan_error(res, "vkAcquireNextImageKHR() failed");

        return imageIdx;
    }
    /// helper to present a swapchain image
    VkResult queuePresentKHR(const vk::Vulkan& vk, VkQueue queue,
            VkSwapchainKHR swapchain, uint32_t imageIdx,
            const vk::Semaphore& waitSemaphore, void* next) {
        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = next,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &waitSemaphore.handle(),
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &imageIdx,
        };
        auto res = vk.df().QueuePresentKHR(queue, &presentInfo);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw ls::vulkan_error(res, "vkQueuePresentKHR() failed");

        return res;
    }
}

std::pair<VkSemaphore, uint64_t> MyVkSwapchain::sync() {
    return { this->waitSemaphore->handle(), this->waitValue };
}

VkResult MyVkSwapchain::present(VkQueue queue, void* next, uint32_t imageIdx) {
    const auto& vk = this->device.get().vkd();

    // wait for completion of copy
    if (this->waitValue > 1 && !this->fence->wait(vk, 150ULL * 1000 * 1000))
        throw ls::vulkan_error(VK_TIMEOUT, "vkWaitForFences() failed");
    this->fence->reset(vk);

    // copy swapchain image into backend source image
    auto& cmdbuf = this->commandBuffer.mut();
    cmdbuf.begin(vk);

    const auto& sync = this->generator.mut().prepare(cmdbuf,
        this->images.at(imageIdx)
    );

    cmdbuf.end(vk);
    cmdbuf.submit(vk,
        {}, this->waitSemaphore->handle(), this->waitValue++,
        {}, sync.first, sync.second,
        this->fence->handle()
    );

    // schedule frame generation
    this->generator.mut().schedule();

    // present generated frames
    for (size_t i = 0; i < this->generator->count(); i++) {
        auto& pass = this->passes.at(this->idx % this->passes.size());

        // acquire swapchain image
        const uint32_t passImageIdx = acquireNextImageKHR(vk, this->handle,
            pass.acquireSemaphore
        );

        // copy backend destination image into swapchain image
        auto& passCmdbuf = pass.commandBuffer;
        passCmdbuf.begin(vk);

        const auto& sync = this->generator.mut().obtain(passCmdbuf,
            this->images.at(passImageIdx)
        );

        passCmdbuf.end(vk);

        std::vector<VkSemaphore> waitSemaphores{ pass.acquireSemaphore.handle() };
        if (i > 0) { // non-first pass
            const auto& prev_pass = this->passes.at((this->idx - 1) % this->passes.size());
            waitSemaphores.push_back(prev_pass.pcs.second.handle());
        }

        const std::vector<VkSemaphore> signalSemaphores{
            pass.pcs.first.handle(),
            pass.pcs.second.handle()
        };

        passCmdbuf.submit(vk,
            waitSemaphores, sync.first, sync.second,
            signalSemaphores, VK_NULL_HANDLE, 0
        );

        // present swapchain image
        queuePresentKHR(vk, queue, this->handle,
            passImageIdx,
            pass.pcs.first,
            next
        );

        next = nullptr; // should only be set for first present
        this->idx++;
    }

    // present original swapchain image
    const auto& prev_pass = this->passes.at((this->idx - 1) % this->passes.size());
    return queuePresentKHR(vk, queue, this->handle,
        imageIdx,
        prev_pass.pcs.second,
        next
    );
}
