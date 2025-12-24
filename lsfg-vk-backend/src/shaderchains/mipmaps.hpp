/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/managed_shader.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace ctx { struct Ctx; }

namespace lsfgvk::backend {
    /// mipmaps shaderchain
    class Mipmaps {
    public:
        /// create a mipmaps shaderchain
        /// @param ctx context
        /// @param sourceImages pair of source images
        Mipmaps(const Ctx& ctx,
            const std::pair<vk::Image, vk::Image>& sourceImages);

        /// prepare the shaderchain initially
        /// @param images vector to fill with image handles
        void prepare(std::vector<VkImage>& images) const;

        /// render the mipmaps shaderchain
        /// @param vk the vulkan instance
        /// @param cmd command buffer
        /// @param idx frame index
        void render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const;

        /// get the generated mipmap images
        /// @return vector of images
        [[nodiscard]] const auto& getImages() const { return this->images; }
    private:
        std::vector<vk::Image> images;

        std::vector<ManagedShader> sets;
        VkExtent2D dispatchExtent{};
    };
}
