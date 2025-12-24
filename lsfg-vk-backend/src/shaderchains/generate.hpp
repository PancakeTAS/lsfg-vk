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
    /// generate shaderchain
    class Generate {
    public:
        /// create a generate shaderchain
        /// @param ctx context
        /// @param idx generated frame index
        /// @param sourceImages pair of source images
        /// @param inputImage1 input image 1
        /// @param inputImage2 input image 2
        /// @param inputImage3 input image 3
        Generate(const Ctx& ctx, size_t idx,
            const std::pair<vk::Image, vk::Image>& sourceImages,
            const vk::Image& inputImage1,
            const vk::Image& inputImage2,
            const vk::Image& inputImage3,
            const vk::Image& outputImage);

        /// render the generate shaderchain
        /// @param vk the vulkan instance
        /// @param cmd command buffer
        /// @param idx frame index
        void render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const;
    private:
        std::vector<ManagedShader> sets;
        VkExtent2D dispatchExtent{};
    };
}
