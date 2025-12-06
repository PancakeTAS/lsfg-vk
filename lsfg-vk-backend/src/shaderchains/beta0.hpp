#pragma once

#include "../helpers/managed_shader.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <vector>

#include <vulkan/vulkan_core.h>

namespace ctx { struct Ctx; }

namespace chains {
    /// beta shaderchain
    class Beta0 {
    public:
        /// create a beta shaderchain
        /// @param ctx context
        /// @param sourceImages source images
        Beta0(const ls::Ctx& ctx,
            const std::vector<std::vector<vk::Image>>& sourceImages);

        /// prepare the shaderchain initially
        /// @param vk vulkan instance
        /// @param cmd command buffer
        void prepare(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const;

        /// render the beta shaderchain
        /// @param vk vulkan instance
        /// @param cmd command buffer
        /// @param idx frame index
        void render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const;

        /// get the generated images
        /// @return vector of images
        [[nodiscard]] const auto& getImages() const { return this->images; }
    private:
        std::vector<vk::Image> images;

        std::vector<ls::ManagedShader> sets;
        VkExtent2D dispatchExtent{};
    };
}
