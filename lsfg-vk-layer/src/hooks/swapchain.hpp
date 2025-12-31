/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "device.hpp"
#include "instance.hpp"
#include "../generator.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/fence.hpp"
#include "lsfg-vk-common/vulkan/semaphore.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    /// swapchain wrapper class
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

        /// reinitialize the swapchain resources
        /// @throws ls::vulkan_error on vulkan errors
        void reinitialize();

        /// get the preset synchronization info
        /// @return pair of wait and signal semaphores
        std::pair<VkSemaphore, uint64_t> sync();

        /// present a frame
        /// @param queue presentation queue
        /// @param next pNext chain pointer
        /// @param imageIdx swapchain image index to present to
        /// @throws ls::vulkan_error on vulkan errors
        VkResult present(VkQueue queue, void* next, uint32_t imageIdx);

        /// get swapchain extent
        /// @return swapchain extent
        [[nodiscard]] VkExtent2D swapchainExtent() const { return this->extent; }
        /// get swapchain format
        /// @return swapchain format
        [[nodiscard]] VkFormat swapchainFormat() const { return this->format; }

        // non-moveable, non-copyable
        MyVkSwapchain(const MyVkSwapchain&) = delete;
        MyVkSwapchain& operator=(const MyVkSwapchain&) = delete;
        MyVkSwapchain(MyVkSwapchain&&) = delete;
        MyVkSwapchain& operator=(MyVkSwapchain&&) = delete;
        ~MyVkSwapchain() = default;
    private:
        ls::R<MyVkLayer> layer;
        ls::R<MyVkInstance> instance;
        ls::R<MyVkDevice> device;

        VkSwapchainKHR handle;
        VkExtent2D extent;
        VkFormat format;
        std::vector<VkImage> images;

        ls::lazy<Generator> generator;

        ls::lazy<vk::TimelineSemaphore> waitSemaphore;
        uint64_t waitValue{1};

        ls::lazy<vk::CommandBuffer> commandBuffer;
        ls::lazy<vk::Fence> fence;

        struct RenderPass {
            vk::CommandBuffer commandBuffer;
            vk::Semaphore acquireSemaphore;

            std::pair<vk::Semaphore, vk::Semaphore> pcs;
        };
        std::vector<RenderPass> passes;
        size_t idx{0};
    };

}
