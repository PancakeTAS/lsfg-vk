#include "lsfg-vk-common/vulkan/shader.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a shader module
    ls::owned_ptr<VkShaderModule> createShaderModule(
            const vk::Vulkan& vk,
            const uint8_t* data, size_t data_len) {
        VkShaderModule handle{};

        const VkShaderModuleCreateInfo shaderModuleInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = data_len,
            .pCode = reinterpret_cast<const uint32_t*>(data)
        };
        auto res = vkCreateShaderModule(vk.dev(), &shaderModuleInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateShaderModule() failed");

        return ls::owned_ptr<VkShaderModule>(
            new VkShaderModule(handle),
            [dev = vk.dev()](VkShaderModule& shaderModule) {
                vkDestroyShaderModule(dev, shaderModule, nullptr);
            }
        );
    }
    /// create a descriptor set layout
    ls::owned_ptr<VkDescriptorSetLayout> createDescriptorSetLayout(
            const vk::Vulkan& vk,
            size_t sampledImages, size_t storageImages,
            size_t buffers, size_t samplers) {
        VkDescriptorSetLayout handle{};

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(buffers + samplers + sampledImages + storageImages);

        for (size_t i = 0; i < buffers; i++)
            bindings.push_back({
                .binding = static_cast<uint32_t>(i),
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            });
        for (size_t i = 0; i < samplers; i++)
            bindings.push_back({
                .binding = static_cast<uint32_t>(i + 16),
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            });
        for (size_t i = 0; i < sampledImages; i++)
            bindings.push_back({
                .binding = static_cast<uint32_t>(i + 32),
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            });
        for (size_t i = 0; i < storageImages; i++)
            bindings.push_back({
                .binding = static_cast<uint32_t>(i + 48),
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            });

        const VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };
        auto res = vkCreateDescriptorSetLayout(vk.dev(), &descriptorLayoutInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateDescriptorSetLayout() failed");

        return ls::owned_ptr<VkDescriptorSetLayout>(
            new VkDescriptorSetLayout(handle),
            [dev = vk.dev()](VkDescriptorSetLayout& layout) {
                vkDestroyDescriptorSetLayout(dev, layout, nullptr);
            }
        );
    }
    /// create a pipeline layout
    ls::owned_ptr<VkPipelineLayout> createPipelineLayout(
            const vk::Vulkan& vk,
            VkDescriptorSetLayout descriptorLayout) {
        VkPipelineLayout handle{};

        const VkPipelineLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &descriptorLayout
        };
        auto res = vkCreatePipelineLayout(vk.dev(), &layoutInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreatePipelineLayout() failed");

        return ls::owned_ptr<VkPipelineLayout>(
            new VkPipelineLayout(handle),
            [dev = vk.dev()](VkPipelineLayout& layout) {
                vkDestroyPipelineLayout(dev, layout, nullptr);
            }
        );
    }
    /// create a compute pipeline
    ls::owned_ptr<VkPipeline> createComputePipeline(
            const vk::Vulkan& vk,
            VkShaderModule shaderModule,
            VkPipelineLayout pipelineLayout) {
        VkPipeline handle{};

        const VkPipelineShaderStageCreateInfo shaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shaderModule,
            .pName = "main"
        };
        const VkComputePipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shaderStageInfo,
            .layout = pipelineLayout
        };
        auto res = vkCreateComputePipelines(vk.dev(),
            VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateComputePipelines() failed");

        // TODO: ponder pipeline cache

        return ls::owned_ptr<VkPipeline>(
            new VkPipeline(handle),
            [dev = vk.dev()](VkPipeline& pipeline) {
                vkDestroyPipeline(dev, pipeline, nullptr);
            }
        );
    }
}

Shader::Shader(const vk::Vulkan& vk, const std::vector<uint8_t>& code,
        size_t sampledImages, size_t storageImages,
        size_t buffers, size_t samplers) :
    shaderModule(createShaderModule(vk,
        code.data(), code.size()
    )),
    descriptorLayout(createDescriptorSetLayout(vk,
        sampledImages, storageImages,
        buffers, samplers
    )),
    pipelineLayout(createPipelineLayout(vk,
        *this->descriptorLayout
    )),
    pipeline(createComputePipeline(vk,
        *this->shaderModule,
        *this->pipelineLayout
    )) {

}
