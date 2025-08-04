#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "core/shadermodule.hpp"
#include "core/device.hpp"
#include "common/exception.hpp"

#include <vector>
#include <cstdint>
#include <utility>
#include <cstddef>
#include <memory>

using namespace LSFG::Core;

ShaderModule::ShaderModule(const Core::Device& device, const std::vector<uint8_t>& code,
        const std::vector<std::pair<size_t, VkDescriptorType>>& descriptorTypes) {
    // create shader module
    const uint8_t* data_ptr = code.data();
    const VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(data_ptr)
    };
    VkShaderModule shaderModuleHandle{};
    auto res = vkCreateShaderModule(device.handle(), &createInfo, nullptr, &shaderModuleHandle);
    if (res != VK_SUCCESS || !shaderModuleHandle)
        throw LSFG::vulkan_error(res, "Failed to create shader module");

    // create descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    size_t bufferIdx{0};
    size_t samplerIdx{16};
    size_t inputIdx{32};
    size_t outputIdx{48};
    for (const auto &[count, type] : descriptorTypes)
        for (size_t i = 0; i < count; i++) {
            size_t* bindIdx{};
            switch (type) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    bindIdx = &bufferIdx;
                    break;
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                    bindIdx = &samplerIdx;
                    break;
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    bindIdx = &inputIdx;
                    break;
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    bindIdx = &outputIdx;
                    break;
                default:
                    throw LSFG::vulkan_error(VK_ERROR_UNKNOWN, "Unsupported descriptor type");
            }

            layoutBindings.emplace_back(VkDescriptorSetLayoutBinding {
                .binding = static_cast<uint32_t>(*bindIdx),
                .descriptorType = type,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            });

            (*bindIdx)++;
        }

    const VkDescriptorSetLayoutCreateInfo layoutDesc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
        .pBindings = layoutBindings.data()
    };
    VkDescriptorSetLayout descriptorSetLayout{};
    res = vkCreateDescriptorSetLayout(device.handle(), &layoutDesc, nullptr, &descriptorSetLayout);
    if (res != VK_SUCCESS || !descriptorSetLayout)
        throw LSFG::vulkan_error(res, "Failed to create descriptor set layout");

    // store module and layout in shared ptr
    this->shaderModule = std::shared_ptr<VkShaderModule>(
        new VkShaderModule(shaderModuleHandle),
        [dev = device.handle()](VkShaderModule* shaderModuleHandle) {
            vkDestroyShaderModule(dev, *shaderModuleHandle, nullptr);
        }
    );
    this->descriptorSetLayout = std::shared_ptr<VkDescriptorSetLayout>(
        new VkDescriptorSetLayout(descriptorSetLayout),
        [dev = device.handle()](VkDescriptorSetLayout* layout) {
            vkDestroyDescriptorSetLayout(dev, *layout, nullptr);
        }
    );
}
