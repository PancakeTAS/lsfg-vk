#pragma once

#include "../helpers/managed_shader.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"

#include <vector>

#include <vulkan/vulkan_core.h>

namespace ctx { struct Ctx; }

namespace chains {
    /// gamma shaderchain
    class Gamma0 {
    public:
        /// create a gamma shaderchain
        /// @param ctx context
        /// @param idx generated frame index
        /// @param sourceImages source images
        /// @param additionalInput additional input image
        Gamma0(const ls::Ctx& ctx, size_t idx,
            const std::vector<std::vector<vk::Image>>& sourceImages,
            const vk::Image& additionalInput);

        /// prepare the shaderchain initially
        /// @param cmd command buffer
        void prepare(const vk::CommandBuffer& cmd) const;

        /// render the gamma shaderchain
        /// @param cmd command buffer
        /// @param idx frame index
        void render(const vk::CommandBuffer& cmd, size_t idx) const;

        /// get the generated images
        /// @return vector of images
        [[nodiscard]] const auto& getImages() const { return this->images; }
    private:
        std::vector<vk::Image> images;

        std::vector<ls::ManagedShader> sets;
        VkExtent2D dispatchExtent{};
    };
}
