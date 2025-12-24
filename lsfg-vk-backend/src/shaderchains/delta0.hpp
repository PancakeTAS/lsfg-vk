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
    /// delta shaderchain
    class Delta0 {
    public:
        /// create a delta shaderchain
        /// @param ctx context
        /// @param idx generated frame index
        /// @param sourceImages source images
        /// @param additionalInput0 additional input image
        /// @param additionalInput1 additional input image
        Delta0(const Ctx& ctx, size_t idx,
            const std::vector<std::vector<vk::Image>>& sourceImages,
            const vk::Image& additionalInput0,
            const vk::Image& additionalInput1);

        /// prepare the shaderchain initially
        /// @param images vector to fill with image handles
        void prepare(std::vector<VkImage>& images) const;

        /// render the delta shaderchain
        /// @param vk the vulkan instance
        /// @param cmd command buffer
        /// @param idx frame index
        void render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const;

        /// get the generated images
        /// @return vector of images
        [[nodiscard]] const auto& getImages0() const { return this->images0; }

        /// get the other generated images
        /// @return vector of images
        [[nodiscard]] const auto& getImages1() const { return this->images1; }
    private:
        std::vector<vk::Image> images0;
        std::vector<vk::Image> images1;

        std::vector<ManagedShader> sets0;
        std::vector<ManagedShader> sets1;
        VkExtent2D dispatchExtent{};
    };
}
