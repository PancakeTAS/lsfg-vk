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
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
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

    // Fill in image sizes in respect to alignment
    for (auto& image : this->m_images) {
        if (image.signature.flags & (ImageFlag::ExternalInput | ImageFlag::ExternalOutput))
            continue; // External inputs have dedicated allocations

        for (const auto& subimage : image.subimages) {
            image.size += vkhelper::align(subimage.memory.size, alignment);
        }
    }

    // Calculate optimal-ish allocations using heuristics & greedy fit strategy
    std::vector<size_t> images(signature.images.size());
    std::iota(images.begin(), images.end(), 0);

    std::ranges::sort(images, [&](const auto& a, const auto& b) {
        return this->m_images.at(a).size > this->m_images.at(b).size;
    });

    std::vector<size_t> placements;
    for (const auto& imageIdx : images) {
        const auto& image{this->m_images.at(imageIdx)};
        if (image.signature.flags & (ImageFlag::ExternalInput | ImageFlag::ExternalOutput))
            continue;

        auto& allocation{
            (image.signature.flags & ImageFlag::Pinned)
                ? this->m_allocations.at(1)
                : this->m_allocations.at(0)
        };
        auto& segment{allocation.segments.emplace_back()};

        vk::DeviceSize size{};
        for (const auto& subimage : image.subimages) {
            const vk::DeviceSize alignedSize{vkhelper::align(subimage.memory.size, alignment)};
            segment.subsegments.push_back({
                .size = alignedSize,
                .offset = size
            });

            size += alignedSize;
        }

        if (image.signature.flags & ImageFlag::Pinned) {
            segment = {
                .imageIdx = imageIdx,
                .subsegments = segment.subsegments,
                .size = size,
                .offset = allocation.size,
            };
            allocation.size += size;
        } else {
            const auto lifetime{image.signature.lifetime};

            vk::DeviceSize offset{};
            for (const auto& otherSegmentIdx : placements) {
                const auto& otherSegment{allocation.segments.at(otherSegmentIdx)};
                if (otherSegment.imageIdx == imageIdx)
                    continue; // Skip self

                const auto& otherImage{this->m_images.at(otherSegment.imageIdx)};
                const auto& otherLifetime{otherImage.signature.lifetime};

                if (lifetime.first > otherLifetime.second ||
                    lifetime.second < otherLifetime.first)
                    continue; // Skip horizontally non-overlapping

                if (offset >= (otherSegment.offset + otherSegment.size) ||
                    otherSegment.offset >= (offset + size))
                    continue; // Skip vertically non-overlapping

                offset = otherSegment.offset + otherSegment.size;
            }

            allocation.size = std::max(allocation.size, offset + size);
            segment = {
                .imageIdx = imageIdx,
                .subsegments = segment.subsegments,
                .size = size,
                .offset = offset,
            };

            const size_t i{allocation.segments.size() - 1};
            auto it{std::ranges::upper_bound(placements, i,
                [&](const auto& a, const auto& b) {
                    return allocation.segments.at(a).offset < allocation.segments.at(b).offset;
                }
            )};
            placements.insert(it, i);
        }
    }

    LOG_DEBUG("  Computed " << this->m_allocations.size() << " memory allocations")

    // Allocate the memory & bind the images
    for (auto& allocation : this->m_allocations) {
        allocation.memory = vkhelper::allocateMemory(
            dld,
            device,
            physdev,
            allocation.size,
            types
        );

        for (const auto& segment : allocation.segments) {
            const auto& image{this->m_images.at(segment.imageIdx)};

            for (size_t i = 0; i < image.subimages.size(); i++) {
                const auto& subsegment{segment.subsegments.at(i)};
                const auto& subimage{image.subimages.at(i)};

                device.bindImageMemory(
                    *subimage.image,
                    *allocation.memory,
                    segment.offset + subsegment.offset,
                    dld
                );
            }
        }

        LOG_DEBUG("  Allocated memory of size " << allocation.size << " for "
            << allocation.segments.size() << " segments")
    }

    // Create image views
    for (auto& image : this->m_images) {
        const bool hasHdrVariant{image.signature.flags & ImageFlag::HdrVariant};
        const bool isLayered{image.subimages.size() == 1 && image.signature.count > 1};

        for (auto& subimage : image.subimages) {
            subimage.view = vkhelper::createImageView(
                dld,
                device,
                *subimage.image,
                static_cast<vk::Format>((hasHdrVariant && hdr)
                    ? image.signature.hdrFormat : image.signature.format),
                isLayered ? image.signature.count : 1
            );
        }
    }

    // Create the descriptor set & required resources
    auto [pool, set] = vkhelper::createDescriptorSet(
        dld,
        device,
        *this->m_layout.layout,
        3, 1, sampledImageCount, storageImageCount
    );
    this->m_descriptorSet.pool = std::move(pool);
    this->m_descriptorSet.set = set;

    const UniformBuffer buf{
        .advancedColorKind = hdr ? 2U : 0U,
        .hdrSupport = hdr ? 1U : 0U,
        .resolutionInvScale = 1.0F / flow,
        .uiThreshold = 0.5F
    };
    this->m_descriptorSet.buffer = vkhelper::createBuffer(
        dld,
        device,
        physdev,
        buf
    );
    auto* mapped{static_cast<UniformBuffer*>(
        device.mapMemory(
            *this->m_descriptorSet.buffer.second,
            0,
            VK_WHOLE_SIZE,
            {},
            dld
        )
    )};
    this->m_descriptorSet.mappedBuffer = std::shared_ptr<UniformBuffer*>(
        new UniformBuffer*{mapped},
        [device, memory = *this->m_descriptorSet.buffer.second, dld](auto* ptr) {
            device.unmapMemory(memory, dld);
            delete ptr; // NOLINT (manual memory management)
        }
    );
    this->m_descriptorSet.samplers.at(0) = vkhelper::createSampler(
        dld,
        device,
        vk::SamplerAddressMode::eClampToBorder,
        vk::CompareOp::eNever,
        false
    );
    this->m_descriptorSet.samplers.at(1) = vkhelper::createSampler(
        dld,
        device,
        vk::SamplerAddressMode::eClampToBorder,
        vk::CompareOp::eNever,
        true
    );
    this->m_descriptorSet.samplers.at(2) = vkhelper::createSampler(
        dld,
        device,
        vk::SamplerAddressMode::eClampToEdge,
        vk::CompareOp::eAlways,
        false
    );

    // Update descriptor set bindings
    std::vector<vk::WriteDescriptorSet> writeInfos(4 + signature.descriptors.size());
    bindingIdx = 0;

    std::array<vk::DescriptorBufferInfo, 1> bufferInfos;
    bufferInfos.at(0) = {
        .buffer = *this->m_descriptorSet.buffer.first,
        .range = VK_WHOLE_SIZE
    };
    writeInfos.at(0) = {
        .dstSet = this->m_descriptorSet.set,
        .dstBinding = bindingIdx++,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = bufferInfos.data()
    };

    std::array<vk::DescriptorImageInfo, 3> samplerInfos;
    for (uint32_t i = 0; i < 3; i++) {
        auto& writeInfo{writeInfos.at(bindingIdx)};

        samplerInfos.at(i) = {
            .sampler = *this->m_descriptorSet.samplers.at(i)
        };
        writeInfo = {
            .dstSet = this->m_descriptorSet.set,
            .dstBinding = bindingIdx++,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampler,
            .pImageInfo = &samplerInfos.at(i)
        };
    }

    std::vector<std::vector<vk::DescriptorImageInfo>> imageInfos2D(signature.descriptors.size());
    for (const auto& binding : signature.descriptors) {
        auto& writeInfo{writeInfos.at(bindingIdx)};

        auto& imageInfos{imageInfos2D.at(bindingIdx - 4)};
        imageInfos.reserve(binding.resources.size());

        for (const auto& resourceIdx : binding.resources) {
            const auto& image{this->m_images.at(resourceIdx)};

            for (const auto& subimage : image.subimages) {
                imageInfos.push_back({
                    .imageView = *subimage.view,
                    .imageLayout = vk::ImageLayout::eGeneral
                });
            }
        }

        writeInfo = {
            .dstSet = this->m_descriptorSet.set,
            .dstBinding = bindingIdx++,
            .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
            .descriptorType = binding.type == BindingType::StorageImage ?
                vk::DescriptorType::eStorageImage : vk::DescriptorType::eSampledImage,
            .pImageInfo = imageInfos.data()
        };
    }

    device.updateDescriptorSets(writeInfos, {}, dld);

    LOG_DEBUG("  Updated descriptor set with " << writeInfos.size() << " bindings")

    // Build all shader pipelines
    std::vector<vk::ComputePipelineCreateInfo> pipelineCreateInfos;
    for (const auto& [name, variant] : signature.shaders) {
        std::string name2{name};
        if (variant) name2 += hdr ? "_16bit" : "_8bit";

        const auto& module{library.shader(name2, perf)};

        pipelineCreateInfos.push_back({
            .stage = {
                .stage = vk::ShaderStageFlagBits::eCompute,
                .module = *module,
                .pName = "main"
            },
            .layout = *this->m_layout.pipelineLayout
        });
    }

    this->m_cache = vkhelper::createPipelineCache(dld, device);
    std::vector<vk::UniquePipeline> pipelines{
        device.createComputePipelinesUnique(
            *this->m_cache,
            pipelineCreateInfos,
            nullptr,
            dld
        ).value
    };

    this->m_pipelines.reserve(signature.shaders.size());
    for (size_t i = 0; i < signature.shaders.size(); i++) {
        const auto& name{signature.shaders.at(i).first};
        this->m_pipelines.emplace(name, std::move(pipelines.at(i)));
    }

    LOG_DEBUG("  Created " << this->m_pipelines.size() << " pipelines")

    LOG_DEBUG("Finished building pipeline")
}
