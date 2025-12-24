/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/descriptor_pool.hpp"
#include "lsfg-vk-common/vulkan/descriptor_set.hpp"
#include "lsfg-vk-common/vulkan/shader.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace lsfgvk::backend {

    /// managed shader handling dispatch and barriers
    /// this class is NOT memory-safe
    class ManagedShader {
        friend class ManagedShaderBuilder;
    public:
        /// dispatch the managed shader
        /// @param vk the vulkan instance
        /// @param cmd command buffer to use
        /// @param extent dispatch size
        /// @throws ls::vulkan_error on failure
        void dispatch(const vk::Vulkan& vk,
            const vk::CommandBuffer& cmd, VkExtent2D extent) const;
    private:
        ls::R<const vk::Shader> shader;

        std::vector<vk::Barrier> barriers;
        vk::DescriptorSet descriptorSet;

        // simple move constructor
        ManagedShader(ls::R<const vk::Shader> shader,
                std::vector<vk::Barrier> barriers,
                vk::DescriptorSet descriptorSet) :
            shader(shader),
            barriers(std::move(barriers)),
            descriptorSet(std::move(descriptorSet)) {
        }
    };

    /// class for building managed shaders
    /// this class is NOT memory-safe
    class ManagedShaderBuilder {
    public:
        /// default constructor
        ManagedShaderBuilder() = default;

        /// add a sampled image
        /// @param image image to add
        [[nodiscard]] ManagedShaderBuilder& sampled(const vk::Image& image);
        /// add multiple sampled images
        /// @param images images to add
        /// @param offset offset into images
        /// @param count number of images to add (0 = all)
        [[nodiscard]] ManagedShaderBuilder& sampleds(const std::vector<vk::Image>& images,
            size_t offset = 0, size_t count = 0);

        /// add a storage image
        /// @param image image to add
        [[nodiscard]] ManagedShaderBuilder& storage(const vk::Image& image);
        /// add multiple storage images
        /// @param images images to add
        /// @param offset offset into images
        /// @param count number of images to add (0 = all)
        [[nodiscard]] ManagedShaderBuilder& storages(const std::vector<vk::Image>& images,
            size_t offset = 0, size_t count = 0);

        /// add a sampler
        /// @param sampler sampler to add
        [[nodiscard]] ManagedShaderBuilder& sampler(const vk::Sampler& sampler);
        /// add multiple samplers
        /// @param samplers samplers to add
        [[nodiscard]] ManagedShaderBuilder& samplers(const std::vector<vk::Sampler>& samplers);

        /// add a buffer
        /// @param buffer buffer to add
        [[nodiscard]] ManagedShaderBuilder& buffer(const vk::Buffer& buffer);

        /// build the managed shader
        /// @param vk the vulkan instance
        /// @param pool the descriptor pool to use
        /// @param shader the shader to use
        /// @returns the built managed shader
        [[nodiscard]] ManagedShader build(const vk::Vulkan& vk,
            const vk::DescriptorPool& pool, const vk::Shader& shader) const;
    private:
        std::vector<ls::R<const vk::Image>> sampledImages;
        std::vector<ls::R<const vk::Image>> storageImages;
        std::vector<ls::R<const vk::Sampler>> imageSamplers;
        std::vector<ls::R<const vk::Buffer>> constantBuffers;
    };

}
