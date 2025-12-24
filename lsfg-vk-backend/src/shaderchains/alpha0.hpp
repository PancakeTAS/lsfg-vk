/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/managed_shader.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <vector>

#include <vulkan/vulkan_core.h>

namespace ctx { struct Ctx; }

namespace lsfgvk::backend {
    /// pre-alpha shaderchain
    class Alpha0 {
    public:
        /// create a pre-alpha shaderchain
        /// @param ctx context
        /// @param sourceImage source image
        Alpha0(const Ctx& ctx,
            const vk::Image& sourceImage);

        /// prepare the shaderchain initially
        /// @param images vector to fill with image handles
        void prepare(std::vector<VkImage>& images) const;

        /// render the pre-alpha shaderchain
        /// @param vk the vulkan instance
        /// @param cmd command buffer
        void render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const;

        /// get the generated images
        /// @return vector of images
        [[nodiscard]] const auto& getImages() const { return this->images; }
    private:
        std::vector<vk::Image> tempImages0;
        std::vector<vk::Image> tempImages1;
        std::vector<vk::Image> images;

        std::vector<ManagedShader> sets;
        VkExtent2D dispatchExtent0{};
        VkExtent2D dispatchExtent1{};
    };
}
