#include "core/descriptorset.hpp"
#include "core/device.hpp"
#include "core/descriptorpool.hpp"
#include "core/shadermodule.hpp"
#include "core/commandbuffer.hpp"
#include "core/pipeline.hpp"
#include "core/image.hpp"
#include "core/sampler.hpp"
#include "core/buffer.hpp"
#include "common/exception.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <cstdint>

using namespace LSFG::Core;

DescriptorSet::DescriptorSet(const Core::Device& device,
        const DescriptorPool& pool, const ShaderModule& shaderModule) {
    // create descriptor set
    VkDescriptorSetLayout layout = shaderModule.getLayout();
    const VkDescriptorSetAllocateInfo desc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool.handle(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };
    VkDescriptorSet descriptorSetHandle{};
    auto res = vkAllocateDescriptorSets(device.handle(), &desc, &descriptorSetHandle);
    if (res != VK_SUCCESS || descriptorSetHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Unable to allocate descriptor set");

    /// store set in shared ptr
    this->descriptorSet = std::shared_ptr<VkDescriptorSet>(
        new VkDescriptorSet(descriptorSetHandle),
        [dev = device.handle(), pool = pool](VkDescriptorSet* setHandle) {
            vkFreeDescriptorSets(dev, pool.handle(), 1, setHandle);
        }
    );
}

DescriptorSetUpdateBuilder DescriptorSet::update(const Core::Device& device) const {
    return { *this, device };
}

void DescriptorSet::bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline) const {
    VkDescriptorSet descriptorSetHandle = this->handle();
    vkCmdBindDescriptorSets(commandBuffer.handle(),
        VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.getLayout(),
        0, 1, &descriptorSetHandle, 0, nullptr);
}

// updater class

DescriptorSetUpdateBuilder& DescriptorSetUpdateBuilder::add(VkDescriptorType type, const Image& image) {
    this->entries.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = this->descriptorSet->handle(),
        .dstBinding = static_cast<uint32_t>(this->entries.size()),
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = new VkDescriptorImageInfo {
            .imageView = image.getView(),
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        },
        .pBufferInfo = nullptr
    });
    return *this;
}

DescriptorSetUpdateBuilder& DescriptorSetUpdateBuilder::add(VkDescriptorType type, const Sampler& sampler) {
    this->entries.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = this->descriptorSet->handle(),
        .dstBinding = static_cast<uint32_t>(this->entries.size()),
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = new VkDescriptorImageInfo {
            .sampler = sampler.handle(),
        },
        .pBufferInfo = nullptr
    });
    return *this;
}

DescriptorSetUpdateBuilder& DescriptorSetUpdateBuilder::add(VkDescriptorType type, const Buffer& buffer) {
    this->entries.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = this->descriptorSet->handle(),
        .dstBinding = static_cast<uint32_t>(this->entries.size()),
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = nullptr,
        .pBufferInfo = new VkDescriptorBufferInfo {
            .buffer = buffer.handle(),
            .range = buffer.getSize()
        }
    });
    return *this;
}

DescriptorSetUpdateBuilder& DescriptorSetUpdateBuilder::add(VkDescriptorType type) {
    this->entries.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = this->descriptorSet->handle(),
        .dstBinding = static_cast<uint32_t>(this->entries.size()),
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = new VkDescriptorImageInfo {
        },
        .pBufferInfo = nullptr
    });
    return *this;
}

void DescriptorSetUpdateBuilder::build() {
    vkUpdateDescriptorSets(this->device->handle(),
        static_cast<uint32_t>(this->entries.size()),
        this->entries.data(), 0, nullptr);

    // NOLINTBEGIN
    for (const auto& entry : this->entries) {
        delete entry.pImageInfo;
        delete entry.pBufferInfo;
    }
    // NOLINTEND
}
