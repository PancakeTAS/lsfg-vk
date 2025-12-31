/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "hooks/device.hpp"
#include "hooks/layer.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    /// frame generation context wrapper
    class Generator {
    public:
        /// create a new frame generation context
        /// @param layer parent layer
        /// @param device parent device
        /// @param extent swapchain extent
        /// @param format swapchain format
        Generator(MyVkLayer& layer, MyVkDevice& device,
            VkExtent2D extent, VkFormat format);

        /// copy a swapchain image into the backend source image
        /// @param cmdbuf command buffer to record into
        /// @param swapchainImage swapchain image to copy from
        /// @return timeline pair to signal for frame generation to start
        /// @throws ls::vulkan_error on vulkan errors
        std::pair<VkSemaphore, uint64_t> prepare(vk::CommandBuffer& cmdbuf, VkImage swapchainImage);

        /// schedule frame generation and increment frame index
        /// @throws ls::error on scheduling errors
        void schedule();

        /// copy a backend destination image into a swapchain image
        /// @param cmdbuf command buffer to record into
        /// @param swapchainImage swapchain image to copy into
        /// @return timeline pair to wait on for frame generation completion
        /// @throws ls::vulkan_error on vulkan errors
        std::pair<VkSemaphore, uint64_t> obtain(vk::CommandBuffer& cmdbuf, VkImage swapchainImage);

        /// return the amount of generated frames
        /// @return generated frames count
        [[nodiscard]] uint64_t count() const { return this->destinationImages.size(); }
    private:
        ls::R<backend::Instance> instance;
        ls::R<const vk::Vulkan> vk;

        std::vector<vk::Image> sourceImages;
        std::vector<vk::Image> destinationImages;
        uint64_t frameIdx{0}; // real frames-only
        uint64_t generatedIdx{0}; // generated frames-only

        ls::lazy<vk::TimelineSemaphore> syncSemaphore;
        uint64_t syncValue{1};

        ls::owned_ptr<ls::R<backend::Context>> ctx;
    };

}
