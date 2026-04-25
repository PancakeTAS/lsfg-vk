/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "pipeline.hpp"
#include "library.hpp"
#include "modules/pipeline/signature.hpp"
#include "modules/pipeline/signature/helpers.hpp"
#include "modules/pipeline/signature/image.hpp"
#include "utility/logger.hpp"
#include "utility/vkhelper.hpp"

#include <cstdint>
#include <utility>
#include <vector>

using namespace lsfgvk::pipeline;

namespace {
    /// Helper method to apply extent operations
    vk::Extent2D apply(
        const vk::Extent2D& base,
        const vk::Extent2D& flow,
        const ExtentOp& op
    ) {
        vk::Extent2D result{op.flow() ? flow : base};
        for (const auto& [add, shift] : op.operations()) {
            result.width = (result.width + add) >> shift;
            result.height = (result.height + add) >> shift;
        }
        return { result.width, result.height };
    }
}

Pipeline::Pipeline(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::PhysicalDevice& physdev,
    const vk::Queue& queue,
    uint32_t queueFamilyIndex,
    const library::ShaderLibrary& library,
    const PipelineSignature& signature,
    vk::Extent2D extent,
    float flow,
    bool perf,
    bool hdr
) {
    LOG_DEBUG("Building pipeline for "
        << extent.width << "x" << extent.height
        << " at " << flow << " flow")

    // Build the Vulkan descriptor set layout
    uint32_t sampledImageCount{};
    uint32_t storageImageCount{};

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(4 + signature.descriptors.size());

    bindings.push_back({
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute
    });

    for (uint32_t i = 1; i <= 3; i++) {
        bindings.push_back({
            .binding = i,
            .descriptorType = vk::DescriptorType::eSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
        });
    }

    uint32_t bindingIdx{4};
    for (const auto& binding : signature.descriptors) {
        uint32_t descriptorCount{static_cast<uint32_t>(binding.resources.size())};
        if (descriptorCount == 1) {
            const auto& image{signature.images.at(binding.resources.front())};
            if (image.flags & ImageFlag::Mipmaps)
                descriptorCount = image.count;
        }

        bindings.push_back({
            .binding = bindingIdx++,
            .descriptorType = binding.type == BindingType::StorageImage ?
                vk::DescriptorType::eStorageImage : vk::DescriptorType::eSampledImage,
            .descriptorCount = descriptorCount,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
        });

        if (binding.type == BindingType::StorageImage)
            storageImageCount += descriptorCount;
        else
            sampledImageCount += descriptorCount;
    }

    auto [layout, pipelineLayout] = vkhelper::createLayout(
        dld,
        device,
        bindings,
        sizeof(PushConstants)
    );
    this->m_layout = {
        .layout = std::move(layout),
        .pipelineLayout = std::move(pipelineLayout)
    };

    LOG_DEBUG("  Built descriptor set layout with " << bindings.size() << " bindings ("
        << sampledImageCount << " sampled images, "
        << storageImageCount << " storage images)")

    LOG_DEBUG("Finished building pipeline")
}
