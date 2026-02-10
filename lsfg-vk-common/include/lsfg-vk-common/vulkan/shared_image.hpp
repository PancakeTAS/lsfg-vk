/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan image
    class SharedImage {
    public:
        /// create an exported image
        /// @param vk the vulkan instance
        /// @param extent extent of the image in pixels
        /// @param format vulkan format of the image
        /// @param usage usage flags
        /// @param drmModifiers list of supported drm format modifiers
        /// @param fd output parameter for the dma-buf fd of the image memory
        /// @throws ls::vulkan_error on failure
        SharedImage(const vk::Vulkan& vk,
            VkExtent2D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            const std::vector<uint64_t>& drmModifiers,
            int& fd);

        /// create an imported image
        /// @param vk the vulkan instance
        /// @param extent extent of the image in pixels
        /// @param format vulkan format of the image
        /// @param usage usage flags
        /// @param drmModifier the drm format modifier of the image
        /// @param drmLayouts the drm offsets and row pitches of the image
        /// @param fd the dma-buf fd to import the image memory from
        /// @throws ls::vulkan_error on failure
        SharedImage(const vk::Vulkan& vk,
            VkExtent2D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            uint64_t drmModifier,
            const std::vector<std::pair<uint64_t, uint64_t>>& drmLayouts,
            int fd);

        /// get the image handle
        /// @return the image handle
        [[nodiscard]] const auto& handle() const { return this->image.get(); }
        /// get the image view handle
        /// @return the image view handle
        [[nodiscard]] const auto& imageview() const { return this->view.get(); }

        /// get the extent of the image
        /// @return the extent of the image
        [[nodiscard]] VkExtent2D getExtent() const { return this->extent; }

        /// get the underlying drm format modifier of the image
        /// @return the drm format modifier of the image
        [[nodiscard]] auto drmModifier() const { return this->modifier; }
        /// get the underlying drm offsets and row pitches of the image
        /// @return the drm offsets and row pitches of the image
        [[nodiscard]] const auto& drmLayouts() const { return this->layouts; }
    private:
        ls::owned_ptr<VkImage> image;
        ls::owned_ptr<VkDeviceMemory> memory;
        ls::owned_ptr<VkImageView> view;

        VkExtent2D extent{};

        uint64_t modifier{};
        std::vector<std::pair<uint64_t, uint64_t>> layouts;
    };
}
