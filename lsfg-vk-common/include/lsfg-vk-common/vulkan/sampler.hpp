/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan sampler
    class Sampler {
    public:
        /// create a sampler
        /// @param vk the vulkan instance
        /// @param mode address mode
        /// @param compare compare operation
        /// @param white whether the border color is white
        /// @throws ls::vulkan_error on failure
        Sampler(const vk::Vulkan& vk,
            VkSamplerAddressMode mode, VkCompareOp compare, bool white);

        /// get the sampler handle
        /// @return the sampler handle
        [[nodiscard]] const auto& handle() const { return this->sampler.get(); }
    private:
        ls::owned_ptr<VkSampler> sampler;
    };
}
