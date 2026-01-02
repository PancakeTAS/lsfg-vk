/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "device.hpp"
#include "instance.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    /// helper struct for partial present information
    struct MyVkPresentInfo {
        uint32_t idx{};
        // VK_KHR_swapchain_maintenance1
        VkFence fence{};
        std::optional<VkPresentModeKHR> present_mode;
        // VK_KHR_present_id(2)
        std::optional<uint64_t> id;
    };

    /// swapchain wrapper (and virtual) class
    class MyVkSwapchain {
    public:
        /// create a vulkan swapchain wrapper
        /// @param layer layer reference
        /// @param instance parent vulkan instance
        /// @param device parent vulkan device
        /// @param info base swapchain info
        /// @param createFunc function to create the swapchain
        /// @throws ls::vulkan_error on vulkan errors
        MyVkSwapchain(MyVkLayer& layer, MyVkInstance& instance, MyVkDevice& device,
            VkSwapchainCreateInfoKHR info,
            const std::function<VkSwapchainKHR(VkSwapchainCreateInfoKHR*)>& createFunc);
        /// reinitialize the contained swapchain resources
        /// @throws ls::vulkan_error on vulkan errors
        void reinitialize();

        /// get the synchronization pair for presentation
        /// @return semaphore and value pair
        [[nodiscard]] std::pair<VkSemaphore, uint64_t> sync();

        /// reimplement vkGetSwapchainImagesKHR
        VkResult GetSwapchainImagesKHR(uint32_t* count, VkImage* images) noexcept;
        /// reimplement vkAcquireNextImageKHR
        VkResult AcquireNextImageKHR(uint64_t timeout,
            VkSemaphore semaphore, VkFence fence, uint32_t* idx) noexcept;
        /// reimplement vkAcquireNextImage2KHR
        VkResult AcquireNextImage2KHR(const VkAcquireNextImageInfoKHR* info,
            uint32_t* idx) noexcept;
        /// reimplement vkQueuePresentKHR
        VkResult partial_QueuePresentKHR(const MyVkPresentInfo& info) noexcept;
        /// reimplement vkReleaseSwapchainImagesKHR
        VkResult ReleaseSwapchainImagesKHR(const VkReleaseSwapchainImagesInfoKHR* info) noexcept;
        /// reimplement vkWaitForPresentKHR
        VkResult WaitForPresentKHR(uint64_t id, uint64_t timeout) noexcept;
        /// reimplement vkWaitForPresent2KHR
        VkResult WaitForPresent2KHR(const VkPresentWait2InfoKHR* info) noexcept;

        /// get the underlying real swapchain handle
        /// @return real swapchain handle
        [[nodiscard]] auto swapchain() const noexcept { return this->handle; }

        // non-moveable, non-copyable
        MyVkSwapchain(const MyVkSwapchain&) = delete;
        MyVkSwapchain& operator=(const MyVkSwapchain&) = delete;
        MyVkSwapchain(MyVkSwapchain&&) = delete;
        MyVkSwapchain& operator=(MyVkSwapchain&&) = delete;
        ~MyVkSwapchain() noexcept;
    private:
        ls::R<MyVkLayer> layer;
        ls::R<MyVkInstance> instance;
        ls::R<MyVkDevice> device;

        vk::TimelineSemaphore presentSemaphore;
        uint64_t presentIndex;

        std::mutex swapchainMutex;
        VkSwapchainKHR handle; // from real swapchain
        std::vector<VkImage> swapchainImages;

        std::vector<vk::Image> images; // virtual swapchain images
        std::mutex availabilityMutex;
        std::vector<bool> availableImages;
        //std::mutex presentationMutex; -- should not be necessary!
        std::queue<MyVkPresentInfo> presents;
        ls::lazy<vk::TimelineSemaphore> doneSemaphore;

        std::atomic_bool running{true};
        std::atomic<VkResult> status{VK_SUCCESS};
        std::thread thread;
        void thread_main() noexcept;
    };

}
