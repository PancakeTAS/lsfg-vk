#include "core/pipeline.hpp"
#include "core/device.hpp"
#include "core/shadermodule.hpp"
#include "core/commandbuffer.hpp"
#include "common/exception.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

using namespace LSFG::Core;

Pipeline::Pipeline(const Core::Device& device, const ShaderModule& shader) {
    // create pipeline layout
    VkDescriptorSetLayout shaderLayout = shader.getLayout();
    const VkPipelineLayoutCreateInfo layoutDesc{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &shaderLayout,
    };
    VkPipelineLayout layoutHandle{};
    auto res = vkCreatePipelineLayout(device.handle(), &layoutDesc, nullptr, &layoutHandle);
    if (res != VK_SUCCESS || !layoutHandle)
        throw LSFG::vulkan_error(res, "Failed to create pipeline layout");

    // create pipeline
    const VkPipelineShaderStageCreateInfo shaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader.handle(),
        .pName = "main",
    };
    const VkComputePipelineCreateInfo pipelineDesc{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shaderStageInfo,
        .layout = layoutHandle,
    };
    VkPipeline pipelineHandle{};
    res = vkCreateComputePipelines(device.handle(),
        VK_NULL_HANDLE, 1, &pipelineDesc, nullptr, &pipelineHandle);
    if (res != VK_SUCCESS || !pipelineHandle)
        throw LSFG::vulkan_error(res, "Failed to create compute pipeline");

    // store layout and pipeline in shared ptr
    this->layout = std::shared_ptr<VkPipelineLayout>(
        new VkPipelineLayout(layoutHandle),
        [dev = device.handle()](VkPipelineLayout* layout) {
            vkDestroyPipelineLayout(dev, *layout, nullptr);
        }
    );
    this->pipeline = std::shared_ptr<VkPipeline>(
        new VkPipeline(pipelineHandle),
        [dev = device.handle()](VkPipeline* pipeline) {
            vkDestroyPipeline(dev, *pipeline, nullptr);
        }
    );
}

void Pipeline::bind(const CommandBuffer& commandBuffer) const {
     vkCmdBindPipeline(commandBuffer.handle(), VK_PIPELINE_BIND_POINT_COMPUTE, *this->pipeline);
}
