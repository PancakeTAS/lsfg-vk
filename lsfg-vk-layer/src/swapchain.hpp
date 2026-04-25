/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "lsfg-vk/lsfgvk.hpp"
#include "lsfg-vk-common/configuration/config.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/fence.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/semaphore.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    ///
    /// Swapchain info struct
    ///
    struct SwapchainInfo {
        std::vector<VkImage> images;
        VkFormat format;
        VkColorSpaceKHR colorSpace;
        VkExtent2D extent;
        VkPresentModeKHR presentMode;
    };

    ///
    /// Modify the swapchain create info based on the profile pre-swapchain creation
    ///
    /// @param profile Active game profile
    /// @param maxImages Maximum number of images supported by the surface
    /// @param createInfo Swapchain create info to modify
    ///
    void context_ModifySwapchainCreateInfo(const ls::GameConf& profile, uint32_t maxImages,
        VkSwapchainCreateInfoKHR& createInfo);

    ///
    /// Swapchain context for a layer instance
    ///
    class Swapchain {
    public:
        ///
        /// Create a new swapchain context
        ///
        /// @param vk Vulkan instance
        /// @param backend lsfg-vk backend instance
        /// @param profile Active game profile
        /// @param info Swapchain info
        ///
        Swapchain(const vk::Vulkan& vk, lsfgvk::Instance& backend,
            ls::GameConf profile, SwapchainInfo info);

        ///
        /// Present a frame
        ///
        /// @param vk Vulkan instance
        /// @param queue Presentation queue
        /// @param next_chain next chain pointer for the present info (WARNING: shared!)
        /// @param imageIdx Swapchain image index to present to
        /// @param semaphores Semaphores to wait on before presenting
        /// @throws ls::vulkan_error on vulkan error
        ///
        VkResult present(const vk::Vulkan& vk,
            VkQueue queue, VkSwapchainKHR swapchain,
            void* next_chain, uint32_t imageIdx,
            const std::vector<VkSemaphore>& semaphores);
    private:
        ls::lazy<vk::Image> sourceImage;
        ls::lazy<vk::Image> destinationImage;
        ls::lazy<vk::TimelineSemaphore> syncSemaphore;

        ls::lazy<vk::CommandBuffer> renderCommandBuffer;
        ls::lazy<vk::Fence> renderFence;
        ls::lazy<vk::Semaphore> finalSemaphore;
        struct RenderPass {
            vk::CommandBuffer commandBuffer;
            vk::Semaphore acquireSemaphore;
            vk::Semaphore copySemaphore;
        };
        std::vector<RenderPass> passes;

        ls::R<lsfgvk::Instance> instance;
        std::unique_ptr<lsfgvk::Context> ctx;
        uint32_t total{};

        size_t iteration{0};
        size_t syncValue{1};

        ls::GameConf profile;
        SwapchainInfo info;
    };

}
