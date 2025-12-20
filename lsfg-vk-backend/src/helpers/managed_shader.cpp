#include "managed_shader.hpp"
#include "lsfg-vk-common/vulkan/buffer.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/descriptor_pool.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/sampler.hpp"
#include "lsfg-vk-common/vulkan/shader.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace ls;

ManagedShaderBuilder& ManagedShaderBuilder::sampled(const vk::Image& image) {
    this->sampledImages.push_back(std::ref(image));
    return *this;
}

ManagedShaderBuilder& ManagedShaderBuilder::sampleds(
        const std::vector<vk::Image>& images,
        size_t offset, size_t count) {
    if (count == 0 || offset + count > images.size())
        count = images.size() - offset;

    for (size_t i = 0; i < count; ++i)
        this->sampledImages.push_back(std::ref(images[offset + i]));
    return *this;
}


ManagedShaderBuilder& ManagedShaderBuilder::storage(const vk::Image& image) {
    this->storageImages.push_back(std::ref(image));
    return *this;
}

ManagedShaderBuilder& ManagedShaderBuilder::storages(
        const std::vector<vk::Image>& images,
        size_t offset, size_t count) {
    if (count == 0 || offset + count > images.size())
        count = images.size() - offset;

    for (size_t i = 0; i < count; ++i)
        this->storageImages.push_back(std::ref(images[offset + i]));
    return *this;
}

ManagedShaderBuilder& ManagedShaderBuilder::sampler(const vk::Sampler& sampler) {
    this->imageSamplers.push_back(std::ref(sampler));
    return *this;
}

ManagedShaderBuilder& ManagedShaderBuilder::samplers(
        const std::vector<vk::Sampler>& samplers) {
    for (const auto& sampler : samplers)
        this->imageSamplers.push_back(std::ref(sampler));
    return *this;
}

ManagedShaderBuilder& ManagedShaderBuilder::buffer(const vk::Buffer& buffer) {
    this->constantBuffers.push_back(std::ref(buffer));
    return *this;
}

ManagedShader ManagedShaderBuilder::build(const vk::Vulkan& vk,
        const vk::DescriptorPool& pool, const vk::Shader& shader) const {
    std::vector<vk::Barrier> barriers;
    barriers.reserve(this->storageImages.size() + this->sampledImages.size());

    for (const auto& img : this->sampledImages)
        barriers.push_back({
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = img.get().handle(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        });
    for (const auto& img : this->storageImages)
        barriers.push_back({
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = img.get().handle(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        });

    return {
        std::ref(shader),
        std::move(barriers),
        vk::DescriptorSet(vk, pool, shader,
            this->sampledImages,
            this->storageImages,
            this->imageSamplers,
            this->constantBuffers)
    };
}

void ManagedShader::dispatch(const vk::Vulkan& vk, const vk::CommandBuffer& cmd,
        VkExtent2D extent) const {
    cmd.dispatch(vk, this->shader,
        this->descriptorSet,
        this->barriers,
        extent.width, extent.height, 1
    );
}
