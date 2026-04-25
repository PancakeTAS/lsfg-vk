/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "pipeline.hpp"
#include "library.hpp"
#include "modules/pipeline/signature.hpp"
#include "modules/pipeline/signature/helpers.hpp"
#include "modules/pipeline/signature/image.hpp"
#include "utility/logger.hpp"
#include "utility/vkhelper.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <stdexcept>
#include <unordered_map>
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

    // Create the Vulkan images
    size_t alignment{};
    uint32_t types{~0U};

    const vk::Extent2D flowExtent{
        static_cast<uint32_t>(static_cast<float>(extent.width) * flow),
        static_cast<uint32_t>(static_cast<float>(extent.height) * flow)
    };
    for (const auto& imageSignature : signature.images) {
        const auto imageIdx{this->m_images.size()};
        auto& image{this->m_images.emplace_back()};
        image = {
            .signature = imageSignature
        };

        const bool hasHdrVariant{image.signature.flags & ImageFlag::HdrVariant};
        const vk::Format format{
            (hasHdrVariant && hdr) ?
                static_cast<vk::Format>(image.signature.hdrFormat) :
                static_cast<vk::Format>(image.signature.format)
        };
        const vk::Extent2D baseExtent{apply(extent, flowExtent, image.signature.extentOp)};
        const vk::ImageUsageFlags usage{
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        };

        const bool isMipmapped{image.signature.flags & ImageFlag::Mipmaps};
        for (uint32_t i = 0; i < image.signature.count; i++) {
            const vk::Extent2D imageExtent{
                .width = std::max(baseExtent.width >> i, 1U),
                .height = std::max(baseExtent.height >> i, 1U)
            };

            if (image.signature.flags & (ImageFlag::ExternalInput | ImageFlag::ExternalOutput)) {
                const bool isInputOr{image.signature.flags & ImageFlag::ExternalInput};

                auto [subimage, allocation] = vkhelper::createExternalImage(
                    dld,
                    device,
                    physdev,
                    imageExtent,
                    format,
                    image.signature.count,
                    usage |
                        (isInputOr ?
                            vk::ImageUsageFlagBits::eTransferDst
                          : vk::ImageUsageFlagBits::eTransferSrc)
                );

                if (isInputOr) {
                    this->m_externalInputs.push_back({
                        .extent = imageExtent,
                        .format = format,
                        .layers = image.signature.count,
                        .image = *subimage,
                        .memory = *allocation
                    });
                } else {
                    this->m_externalOutputs.push_back({
                        .extent = imageExtent,
                        .format = format,
                        .layers = image.signature.count,
                        .image = *subimage,
                        .memory = *allocation
                    });
                }

                LOG_DEBUG("  Allocated memory of size "
                    << [&]() {
                        const auto& reqs{device.getImageMemoryRequirements(*subimage, dld)};
                        return reqs.size;
                    }() << " for external image " << imageIdx)

                image.subimages.push_back({
                    .image = std::move(subimage)
                });
                this->m_externalAllocations[imageIdx] = std::move(allocation);

                break; // There can only be one image
            }

            image.subimages.push_back({
                .image = vkhelper::createImage(
                    dld,
                    device,
                    imageExtent,
                    format,
                    isMipmapped ? 1 : image.signature.count,
                    usage
                )
            });

            if (!isMipmapped) {
                break;
            }
        }

        for (auto& subimage : image.subimages) {
            subimage.memory = device.getImageMemoryRequirements(*subimage.image, dld);

            if (image.signature.flags & (ImageFlag::ExternalInput | ImageFlag::ExternalOutput))
                break;

            alignment = std::max(alignment, subimage.memory.alignment);
            types &= subimage.memory.memoryTypeBits;
        }
    }

    if (types == 0)
        throw std::runtime_error("No compatible memory type found for pipeline images");

    LOG_DEBUG("  Created " << this->m_images.size() << " images with common alignment "
        << alignment << " and memory type bits " << std::hex << types << std::dec)

    LOG_DEBUG("Finished building pipeline")
}
