/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/vulkan/descriptor_pool.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <array>
#include <cstdint>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a descriptor pool
    ls::owned_ptr<VkDescriptorPool> createDescriptorPool(const vk::Vulkan& vk,
            const Limits& limits) {
        VkDescriptorPool handle{};

        const std::array<VkDescriptorPoolSize, 4> poolCounts{{
            {
                .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = limits.samplers
            },
            {
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = limits.sampled_images
            },
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = limits.storage_images
            },
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = limits.uniform_buffers
            }
        }};
        const VkDescriptorPoolCreateInfo descpoolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = limits.sets,
            .poolSizeCount = static_cast<uint32_t>(poolCounts.size()),
            .pPoolSizes = poolCounts.data()
        };
        auto res = vk.df().CreateDescriptorPool(vk.dev(), &descpoolInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateDescriptorPool() failed");

        return ls::owned_ptr<VkDescriptorPool>(
            new VkDescriptorPool(handle),
            [dev = vk.dev(), defunc = vk.df().DestroyDescriptorPool](VkDescriptorPool& pool) {
                defunc(dev, pool, nullptr);
            }
        );
    }
}

DescriptorPool::DescriptorPool(const vk::Vulkan& vk, const Limits& limits)
    : descriptor_pool(createDescriptorPool(vk, limits)) {}
