/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <optional>

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan image
    class Image {
    public:
        /// create an image
        /// @param vk the vulkan instance
        /// @param extent extent of the image in pixels
        /// @param format vulkan format of the image
        /// @param usage usage flags
        /// @param importFd optional file descriptor for shared memory
        /// @param exportFd optional pointer to an integer where the file descriptor will be stored
        /// @throws ls::vulkan_error on failure
        Image(const vk::Vulkan& vk,
            VkExtent2D extent,
            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
            VkImageUsageFlags usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            std::optional<int> importFd = std::nullopt,
            std::optional<int*> exportFd = std::nullopt);

        /// get the image handle
        /// @return the image handle
        [[nodiscard]] const auto& handle() const { return this->image.get(); }
        /// get the image view handle
        /// @return the image view handle
        [[nodiscard]] const auto& imageview() const { return this->view.get(); }

        /// get the extent of the image
        /// @return the extent of the image
        [[nodiscard]] VkExtent2D getExtent() const { return this->extent; }
    private:
        ls::owned_ptr<VkImage> image;
        ls::owned_ptr<VkDeviceMemory> memory;
        ls::owned_ptr<VkImageView> view;

        VkExtent2D extent{};
    };
}
