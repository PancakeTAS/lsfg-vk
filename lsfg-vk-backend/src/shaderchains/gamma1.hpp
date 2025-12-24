/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/managed_shader.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <vector>

#include <vulkan/vulkan_core.h>

namespace ctx { struct Ctx; }

namespace lsfgvk::backend {
    /// gamma shaderchain
    class Gamma1 {
    public:
        /// create a gamma shaderchain
        /// @param ctx context
        /// @param idx generated frame index
        /// @param sourceImages source images
        /// @param additionalInput0 additional input image
        /// @param additionalInput1 additional input image
        Gamma1(const Ctx& ctx, size_t idx,
            const std::vector<vk::Image>& sourceImages,
            const vk::Image& additionalInput0,
            const vk::Image& additionalInput1);

        /// prepare the shaderchain initially
        /// @param images vector to fill with image handles
        void prepare(std::vector<VkImage>& images) const;

        /// render the gamma shaderchain
        /// @param vk the vulkan instance
        /// @param cmd command buffer
        void render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const;

        /// get the generated image
        /// @return image
        [[nodiscard]] const auto& getImage() const { return *this->image; }
    private:
        std::vector<vk::Image> tempImages0;
        std::vector<vk::Image> tempImages1;
        ls::lazy<vk::Image> image;

        std::vector<ManagedShader> sets;
        VkExtent2D dispatchExtent{};
    };
}
