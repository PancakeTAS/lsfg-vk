/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "device.hpp"
#include "instance.hpp"
#include "layer.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/fence.hpp"
#include "lsfg-vk-common/vulkan/semaphore.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"
#include "swapchain.hpp"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <time.h>
#include <unistd.h>
#include <bits/time.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk;
using namespace lsfgvk::layer;

namespace {
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
        presentSemaphore(device.vkd(), 0), presentIndex(1) {
    const auto& vk = device.vkd();

    // ensure underlying swapchain supports transfer operations
    info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // check for VK_KHR_swapchain_mutable_format
    VkImageCreateFlags flags{};
    if (info.flags & VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR)
        flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    const auto* chain = ls::find_structure<VkImageFormatListCreateInfo>(
        VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO,
        info.pNext
    );

    // create underlying swapchain
    this->handle = createFunc(&info);
    this->swapchainImages = getSwapchainImages(vk, this->handle);

    // create virtual swapchain images
    this->images.reserve(this->swapchainImages.size());
    this->availableImages = std::vector<bool>(this->swapchainImages.size(), true);
    for (size_t i = 0; i < this->swapchainImages.size(); i++) {
        this->images.emplace_back(vk,
            info.imageExtent,
            info.imageFormat,
            info.imageUsage,
            std::nullopt, std::nullopt,
            flags,
            reinterpret_cast<const void*>(chain)
        );
    }

    // create thread
    this->doneSemaphore.emplace(vk, 0);
    this->thread = std::thread(&MyVkSwapchain::thread_main, this);

    // this->reinitialize();
}

// void MyVkSwapchain::reinitialize() {
//     // ...
// }

MyVkSwapchain::~MyVkSwapchain() noexcept {
    this->running.store(false);
    if (this->thread.joinable())
        this->thread.join();
}

/* Virtual swapchain logic */

std::pair<VkSemaphore, uint64_t> MyVkSwapchain::sync() {
    return { this->presentSemaphore.handle(), this->presentIndex++ };
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

void MyVkSwapchain::thread_main() noexcept {
    const auto& vk = this->device.get().vkd();
    auto& offload = this->device.get().offload();

    struct Pass {
        vk::Semaphore acquireSemaphore;
        vk::CommandBuffer commandBuffer;
        vk::Fence copyFence;
        vk::Semaphore presentSemaphore;
    };

    std::vector<Pass> passes;
    passes.reserve(this->swapchainImages.size() + 1);
    for (size_t i = 0; i < this->swapchainImages.size() + 1; i++) {
        passes.emplace_back(Pass {
            .acquireSemaphore = vk::Semaphore(vk),
            .commandBuffer = vk::CommandBuffer(vk),
            .copyFence = vk::Fence(vk),
            .presentSemaphore = vk::Semaphore(vk)
        });
    }

    try { // FIXME: indentation and stuff

    uint64_t counter{1};
    while (this->running.load()) {
        // wait for present signal and fetch the image index
        const auto ppi = this->virtual_FetchUPresent(100'1000, counter);
        if (!ppi.has_value())
            continue; // timeout after 100us

        // acquire a real swapchain image
        const auto& pass = passes[counter % passes.size()];
        const uint32_t real_idx = this->virtual_AcquireNext(pass.acquireSemaphore);

        // copy virtual image into real swapchain image
        const auto& cmdbuf = pass.commandBuffer;
        cmdbuf.begin(vk);

        auto& virtualImage = this->images.at(ppi->idx);
        auto& swapchainImage = this->swapchainImages.at(real_idx);

        cmdbuf.blitImage(vk,
            {
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_NONE,
                    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    FILL_BARRIER(virtualImage.handle())
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
            { virtualImage.handle(), swapchainImage },
            virtualImage.getExtent(),
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

        cmdbuf.end(vk);

        {
            const std::scoped_lock<std::mutex> lock(offload.mutex);
            cmdbuf.submit(vk,
                { pass.acquireSemaphore.handle() }, VK_NULL_HANDLE, 0,
                { pass.presentSemaphore.handle() }, VK_NULL_HANDLE, 0,
                pass.copyFence.handle(), offload.queue
            );
        }

        // present the real swapchain image
        this->virtual_PresentLinked(*ppi, pass.presentSemaphore, real_idx);

        // wait for the copy to finish
        if (!pass.copyFence.wait(vk, UINT64_MAX))
            throw ls::error("virtual swapchain copy fence wait timed out");
        pass.copyFence.reset(vk);

        // mark image as available again
        this->virtual_CompleteUPresent(*ppi);
    }

    } catch (const std::exception& e) {
        std::cerr << "lsfg-vk: virtual swapchain encountered an error:\n"
            "- " << e.what() << "\n";
    }

    if (offload.mutex.try_lock()) { // must be in vkDeviceWaitIdle if locked
        vk.df().QueueWaitIdle(offload.queue);
        offload.mutex.unlock();
    }
}

std::optional<MyVkPresentInfo> MyVkSwapchain::virtual_FetchUPresent(uint64_t timeout,
        uint64_t& counter) {
    const auto& vk = this->device.get().vkd();

    if (!this->presentSemaphore.wait(vk, counter, timeout))
        return std::nullopt;
    counter++;

    if (this->presents.empty())
        throw ls::error("virtual_FetchUPresent() encountered an impossible state");

    const auto info = this->presents.front();
    this->presents.pop();

    return info;
}

uint32_t MyVkSwapchain::virtual_AcquireNext(const vk::Semaphore& semaphore) {
    const auto& vk = this->device.get().vkd();

    uint32_t idx{};
    {
        const std::scoped_lock<std::mutex> lock(this->swapchainMutex);

        auto res = vk.df().AcquireNextImageKHR(vk.dev(),
            this->handle, UINT64_MAX,
            semaphore.handle(), VK_NULL_HANDLE,
            &idx
        );
        if (res != VK_SUCCESS) {
            this->status.store(res);

            if (res != VK_SUBOPTIMAL_KHR)
                throw ls::error("vkAcquireNextImageKHR() failed");
        }

    }

    return idx;
}

void MyVkSwapchain::virtual_PresentLinked(const MyVkPresentInfo& original_info,
        const vk::Semaphore& semaphore, uint32_t idx) {
    const auto& vk = this->device.get().vkd();

    const uint64_t presentId = original_info.id.value_or(0);
    const VkPresentIdKHR presentIdInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_ID_KHR,
        .swapchainCount = 1,
        .pPresentIds = &presentId
    };
    const auto mode = original_info.present_mode.value_or(VK_PRESENT_MODE_FIFO_KHR);
    const VkSwapchainPresentModeInfoKHR presentModeInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_KHR,
        .pNext = original_info.id.has_value() ? &presentIdInfo : nullptr,
        .swapchainCount = 1,
        .pPresentModes = &mode
    };

    const void* chain{};
    if (original_info.present_mode.has_value())
        chain = &presentModeInfo;
    else if (original_info.id.has_value())
        chain = &presentIdInfo;
    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = chain,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &semaphore.handle(),
        .swapchainCount = 1,
        .pSwapchains = &this->handle,
        .pImageIndices = &idx,
    };
    {
        auto& offload = this->device.get().offload();

        const std::scoped_lock<std::mutex> lock(offload.mutex);
        const std::scoped_lock<std::mutex> lock2(this->swapchainMutex);

        auto res = vk.df().QueuePresentKHR(offload.queue, &presentInfo);
        if (res != VK_SUCCESS) {
            this->status.store(res);

            if (res != VK_SUBOPTIMAL_KHR)
                throw ls::error("vkQueuePresentKHR() failed");
        }
    }

    if (original_info.id.has_value())
        this->doneSemaphore->signal(vk, presentId + 1);
}

void MyVkSwapchain::virtual_CompleteUPresent(const MyVkPresentInfo& info) {
    const auto& vk = this->device.get().vkd();

    {
        const std::scoped_lock<std::mutex> lock(this->availabilityMutex);
        this->availableImages.at(info.idx) = true;
    }

    if (info.fence != VK_NULL_HANDLE) {
        auto& offload = this->device.get().offload();

        auto res = vk.df().QueueSubmit(offload.queue, 0, nullptr, info.fence);
        if (res != VK_SUCCESS)
            this->status.store(res);
    }
}

/* Reimplementations of Vulkan functions */

VkResult MyVkSwapchain::GetSwapchainImagesKHR(uint32_t *count, VkImage *images) noexcept {
    if (images == nullptr) {
        *count = static_cast<uint32_t>(this->images.size());
        return VK_SUCCESS;
    }

    VkResult res = VK_SUCCESS;
    auto limit = static_cast<uint32_t>(this->images.size());

    if (*count < limit) {
        limit = *count;
        res = VK_INCOMPLETE;
    }

    for (uint32_t i = 0; i < limit; i++)
        images[i] = this->images[i].handle();

    *count = limit;
    return res;
}

namespace {
    /// find and mark an available image
    std::optional<uint32_t> mark_available(std::vector<bool>& avail, std::mutex& mutex) noexcept {
        const std::scoped_lock<std::mutex> lock(mutex);
        for (size_t i = 0; i < avail.size(); i++) {
            if (!avail[i])
                continue;

            avail[i] = false;
            return static_cast<uint32_t>(i);
        }
        return std::nullopt;
    }
}

namespace {
    /// find now in microseconds
    uint64_t nowInUs() noexcept {
        timespec ts{};
        clock_gettime(CLOCK_MONOTONIC, &ts);

        return static_cast<uint64_t>(ts.tv_sec) * 1'000'000
            + static_cast<uint64_t>(ts.tv_nsec) / 1'000;
    }
    /// signal a semaphore and fence
    void signalSemaphoreAndFence(const vk::Vulkan& vk,
            VkSemaphore semaphore, VkFence fence) noexcept {
        const VkSubmitInfo info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .signalSemaphoreCount = semaphore != VK_NULL_HANDLE,
            .pSignalSemaphores = semaphore != VK_NULL_HANDLE ? &semaphore : nullptr
        };
        vk.df().QueueSubmit(vk.queue(), 1, &info, fence);
    }
}

VkResult MyVkSwapchain::AcquireNextImageKHR(uint64_t timeout,
        VkSemaphore semaphore, VkFence fence, uint32_t *idx) noexcept {
    const auto& vk = this->device.get().vkd();

    // fast path: try to get an available image without blocking
    if (auto optIdx = mark_available(this->availableImages, this->availabilityMutex)) {
        *idx = *optIdx;

        signalSemaphoreAndFence(vk, semaphore, fence);
        return this->status.load();
    }

    // if call is non-blocking: return not ready
    if (timeout == 0)
        return VK_NOT_READY;

    // otherwise: repeat fetch and delay 100us
    const uint64_t start = nowInUs();
    timeout *= 1'000; // to microseconds

    while (nowInUs() - start < timeout) {
        auto res = this->status.load();
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            return res;

        if (auto optIdx = mark_available(this->availableImages, this->availabilityMutex)) {
            *idx = *optIdx;

            signalSemaphoreAndFence(vk, semaphore, fence);
            return res;
        }

        usleep(100);
    }

    return VK_TIMEOUT;
}

VkResult MyVkSwapchain::AcquireNextImage2KHR(const VkAcquireNextImageInfoKHR* info,
        uint32_t* idx) noexcept {
    return this->AcquireNextImageKHR(
        info->timeout,
        info->semaphore,
        info->fence,
        idx
    );
}

VkResult MyVkSwapchain::partial_QueuePresentKHR(const MyVkPresentInfo& info) noexcept {
    this->presents.emplace(info);
    return this->status.load();
}

VkResult MyVkSwapchain::ReleaseSwapchainImagesKHR(
        const VkReleaseSwapchainImagesInfoKHR* info) noexcept {
    for (uint32_t i = 0; i < info->imageIndexCount; i++) {
        const uint32_t idx = info->pImageIndices[i];

        const std::scoped_lock<std::mutex> lock(this->availabilityMutex);
        this->availableImages.at(idx) = true;
    }
    return VK_SUCCESS;
}

VkResult MyVkSwapchain::WaitForPresentKHR(uint64_t id, uint64_t timeout) noexcept {
    const auto& vk = this->device.get().vkd();

    if (!this->doneSemaphore->wait(vk, id + 1, timeout))
        return VK_TIMEOUT;

    const std::scoped_lock<std::mutex> lock(this->swapchainMutex);
    const auto& df = vk.df();

    if (df.WaitForPresentKHR) {
        return vk.df().WaitForPresentKHR(vk.dev(), this->handle, id, timeout);
    }

    if (df.WaitForPresent2KHR) {
        const VkPresentWait2InfoKHR info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_WAIT_2_INFO_KHR,
            .presentId = id,
            .timeout = timeout
        };
        return vk.df().WaitForPresent2KHR(vk.dev(), this->handle, &info);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT; // should never happen
}

VkResult MyVkSwapchain::WaitForPresent2KHR(
        const VkPresentWait2InfoKHR* info) noexcept {
    return this->WaitForPresentKHR(
        info->presentId,
        info->timeout
    );
}
