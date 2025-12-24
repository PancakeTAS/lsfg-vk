/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/vulkan/descriptor_set.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/buffer.hpp"
#include "lsfg-vk-common/vulkan/descriptor_pool.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/sampler.hpp"
#include "lsfg-vk-common/vulkan/shader.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a descriptor set
    ls::owned_ptr<VkDescriptorSet> createDescriptorSet(const vk::Vulkan& vk,
            const vk::DescriptorPool& pool, const vk::Shader& shader) {
        VkDescriptorSet handle{};

        auto* layout = shader.descriptorlayout();
        const VkDescriptorSetAllocateInfo setInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pool.handle(),
            .descriptorSetCount = 1,
            .pSetLayouts = &layout
        };
        auto res = vk.df().AllocateDescriptorSets(vk.dev(), &setInfo, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkAllocateDescriptorSets() failed");

        return ls::owned_ptr<VkDescriptorSet>(
            new VkDescriptorSet(handle),
            [dev = vk.dev(), pool = pool.handle(), defunc = vk.df().FreeDescriptorSets](
                VkDescriptorSet& commandBufferModule
            ) {
                defunc(dev, pool, 1, &commandBufferModule);
            }
        );
    }
}

DescriptorSet::DescriptorSet(const vk::Vulkan& vk,
            const vk::DescriptorPool& pool, const vk::Shader& shader,
            const std::vector<ls::R<const vk::Image>>& sampledImages,
            const std::vector<ls::R<const vk::Image>>& storageImages,
            const std::vector<ls::R<const vk::Sampler>>& samplers,
            const std::vector<ls::R<const vk::Buffer>>& buffers)
        : descriptorSet(createDescriptorSet(vk, pool, shader)) {
    // update descriptor set
    const size_t bindingCount =
        samplers.size()
        + sampledImages.size()
        + storageImages.size()
        + buffers.size();

    std::vector<VkWriteDescriptorSet> entries;
    entries.reserve(bindingCount);

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(buffers.size());

    size_t bufferIdx{0};
    for (const auto& buf : buffers)
        entries.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = *this->descriptorSet,
            .dstBinding = static_cast<uint32_t>(bufferIdx++),
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &(bufferInfos.emplace_back(VkDescriptorBufferInfo{
                .buffer = buf.get().handle(),
                .range = buf.get().length()
            }))
        });

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(bindingCount);

    size_t samplerIdx{16};
    for (const auto& samp : samplers)
        entries.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = *this->descriptorSet,
            .dstBinding = static_cast<uint32_t>(samplerIdx++),
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo = &(imageInfos.emplace_back(VkDescriptorImageInfo{
                .sampler = samp.get().handle(),
            }))
        });

    size_t sampledIdx{32};
    for (const auto& img : sampledImages) {
        entries.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = *this->descriptorSet,
            .dstBinding = static_cast<uint32_t>(sampledIdx++),
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &(imageInfos.emplace_back(VkDescriptorImageInfo{
                .imageView = img.get().imageview(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            }))
        });
    }

    size_t storageIdx{48};
    for (const auto& img : storageImages)
        entries.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = *this->descriptorSet,
            .dstBinding = static_cast<uint32_t>(storageIdx++),
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &(imageInfos.emplace_back(VkDescriptorImageInfo{
                .imageView = img.get().imageview(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            }))
        });

    vk.df().UpdateDescriptorSets(vk.dev(),
        static_cast<uint32_t>(entries.size()), entries.data(), 0, nullptr);
}
