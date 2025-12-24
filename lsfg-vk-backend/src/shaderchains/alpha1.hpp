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
    /// alpha shaderchain
    class Alpha1 {
    public:
        /// create a alpha shaderchain
        /// @param ctx context
        /// @param temporal temporal count
        /// @param sourceImages source images
        Alpha1(const Ctx& ctx, size_t temporal,
            const std::vector<vk::Image>& sourceImages);

        /// prepare the shaderchain initially
        /// @param images vector to fill with image handles
        void prepare(std::vector<VkImage>& images) const;

        /// render the alpha shaderchain
        /// @param vk the vulkan instance
        /// @param cmd command buffer
        /// @param idx frame index
        void render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const;

        /// get the generated images
        /// @return vector of images
        [[nodiscard]] const auto& getImages() const { return this->images; }
    private:
        std::vector<std::vector<vk::Image>> images;

        std::vector<ManagedShader> sets;
        VkExtent2D dispatchExtent{};
    };
}
