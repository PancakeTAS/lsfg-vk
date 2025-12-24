/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "buffer.hpp"
#include "image.hpp"
#include "descriptor_pool.hpp"
#include "sampler.hpp"
#include "shader.hpp"
#include "vulkan.hpp"

#include <vector>

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan descriptor set
    class DescriptorSet {
    public:
        /// create a descriptor set
        /// @param vk the vulkan instance
        /// @param pool the descriptor pool to allocate from
        /// @param shader the shader module this descriptor set is for
        /// @param sampledImages the sampled images to bind
        /// @param storageImages the storage images to bind
        /// @param samplers the samplers to bind
        /// @param buffers the buffers to bind
        /// @throws ls::vulkan_error on failure
        DescriptorSet(const vk::Vulkan& vk,
            const vk::DescriptorPool& pool, const vk::Shader& shader,
            const std::vector<ls::R<const vk::Image>>& sampledImages,
            const std::vector<ls::R<const vk::Image>>& storageImages,
            const std::vector<ls::R<const vk::Sampler>>& samplers,
            const std::vector<ls::R<const vk::Buffer>>& buffers);

        /// get the underlying descriptor set handle
        /// @return the handle
        [[nodiscard]] const auto& handle() const { return *this->descriptorSet; }
    private:
        ls::owned_ptr<VkDescriptorSet> descriptorSet;
    };
}
