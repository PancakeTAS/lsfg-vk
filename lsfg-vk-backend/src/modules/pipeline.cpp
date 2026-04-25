/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "pipeline.hpp"
#include "library.hpp"
#include "modules/pipeline/signature.hpp"
#include "modules/pipeline/signature/helpers.hpp"
#include "modules/pipeline/signature/image.hpp"
#include "modules/pipeline/signature/pass.hpp"
#include "utility/vkhelper.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
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

    const std::string_view cacheTag{perf ? "performance" : "quality"};
    auto [cache, isCacheValid] = vkhelper::createPipelineCache(
        dld,
        device,
        physdev,
        cacheTag
    );
    this->m_cache = std::move(cache);

    std::vector<vk::UniquePipeline> pipelines{
        device.createComputePipelinesUnique(
            *this->m_cache,
            pipelineCreateInfos,
            nullptr,
            dld
        ).value
    };

    if (!isCacheValid) {
        vkhelper::persistPipelineCache(
            dld,
            device,
            physdev,
            *this->m_cache,
            cacheTag
        );
    }

    this->m_pipelines.reserve(signature.shaders.size());
    for (size_t i = 0; i < signature.shaders.size(); i++) {
        const auto& name{signature.shaders.at(i).first};
        this->m_pipelines.emplace(name, std::move(pipelines.at(i)));
    }

    // Build pipeline stages
    std::unordered_map<std::string_view, uint32_t> indices;
    for (const auto& stageSignature : signature.stages) {
        auto& stage{this->m_stages.emplace_back()};
        stage.substages.emplace_back();

        for (const auto& passIdx : stageSignature.passes) { // (Sorted by shader)
            const auto& pass{signature.passes.at(passIdx)};

            for (const auto& resource : pass.inputs) {
                if (!resource.idx())
                    continue;
                stage.sampledImages.push_back(*resource.idx());
            }
            for (const auto& resource : pass.outputs) {
                if (!resource.idx())
                    continue;
                stage.storageImages.push_back(*resource.idx());
            }

            auto& lastPipeline{stage.substages.back().pipeline};
            if (!lastPipeline.empty() && lastPipeline != pass.shader) {
                stage.substages.emplace_back();
            }

            auto& substage{stage.substages.back()};
            substage.pipeline = pass.shader;
            substage.subiterations.push_back({
                .iterationIndex = indices[substage.pipeline]++,
                .dispatch = apply(extent, flowExtent, pass.dispatchOp),
                .isSpecial = pass.flags & PassFlag::Special
            });
        }
    }

    // Transition all images into general layout
    this->m_pool = vkhelper::createCommandPool(
        dld,
        device,
        queueFamilyIndex
    );

    std::vector<vk::ImageMemoryBarrier2KHR> barriers;
    for (const auto& image : this->m_images) {
        for (const auto& subimage : image.subimages) {
            barriers.push_back({
                .newLayout = vk::ImageLayout::eGeneral,
                .image = *subimage.image,
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .levelCount = 1,
                    .layerCount = image.subimages.size() == 1 ? image.signature.count : 1
                }
            });
        }
    }

    const auto layoutCmdbuf{
        vkhelper::createCommandBuffer(dld, device, *this->m_pool)
    };

    layoutCmdbuf->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit }, dld);
    layoutCmdbuf->pipelineBarrier2KHR({
        .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    }, dld);
    layoutCmdbuf->end(dld);

    const auto fence{device.createFenceUnique({}, nullptr, dld)};
    queue.submit(
        {{
            .commandBufferCount = 1,
            .pCommandBuffers = &*layoutCmdbuf
        }},
        *fence,
        dld
    );
    if (device.waitForFences(*fence, VK_TRUE, 50'000'000, dld) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for image layout transition fence");
    }

    for (size_t i = 0; i < signature.splitIndices.size() + 1; i++) {
        auto& cmdbuf{this->m_cmdbufs.emplace_back()};
        cmdbuf = vkhelper::createCommandBuffer(dld, device, *this->m_pool);
        cmdbuf->begin({ .flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse }, dld);

        cmdbuf->bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            *this->m_layout.pipelineLayout,
            0,
            this->m_descriptorSet.set,
            {},
            dld
        );
    }

    size_t currentStageIndex{0};
    size_t currentStageBound{
        signature.splitIndices.empty() ? signature.passes.size() : signature.splitIndices.front()
    };

    std::vector<vk::ImageMemoryBarrier2KHR> barrierVector;
    barrierVector.reserve(16);

    std::unordered_map<VkImage, vk::ImageMemoryBarrier2KHR> stageBarriers;
    for (size_t i = 0; i < this->m_stages.size(); i++) {
        if (i == currentStageBound) {
            currentStageIndex++;
            currentStageBound = currentStageIndex < signature.splitIndices.size() ?
                signature.splitIndices.at(currentStageIndex) : signature.passes.size();
        }

        const auto& stage{this->m_stages.at(i)};
        const auto& cmdbuf{this->m_cmdbufs.at(currentStageIndex)};

        // Append barriers for this stage
        for (const auto& sampledImage : stage.sampledImages) {
            const auto& image = this->m_images.at(sampledImage);
            for (const auto& subimage : image.subimages) {
                const vk::Image& imageHandle{*subimage.image};
                if (stageBarriers.contains(imageHandle)) {
                    stageBarriers[imageHandle].dstAccessMask = vk::AccessFlagBits2::eShaderRead;
                    continue;
                }

                stageBarriers[imageHandle] = {
                    .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                    .srcAccessMask = vk::AccessFlagBits2::eNone,
                    .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                    .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                    .image = *subimage.image,
                    .subresourceRange = {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .levelCount = 1,
                        .layerCount = image.subimages.size() == 1 ? image.signature.count : 1
                    }
                };
            }
        }
        for (const auto& storageImage : stage.storageImages) {
            const auto& image = this->m_images.at(storageImage);
            for (const auto& subimage : image.subimages) {
                const vk::Image& imageHandle{*subimage.image};
                if (stageBarriers.contains(imageHandle)) {
                    stageBarriers[imageHandle].dstAccessMask = vk::AccessFlagBits2::eShaderWrite;
                    continue;
                }

                stageBarriers[imageHandle] = {
                    .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                    .srcAccessMask = vk::AccessFlagBits2::eNone,
                    .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                    .dstAccessMask = vk::AccessFlagBits2::eShaderWrite,
                    .image = *subimage.image,
                    .subresourceRange = {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .levelCount = 1,
                        .layerCount = image.subimages.size() == 1 ? image.signature.count : 1
                    }
                };
            }
        }


        barrierVector.clear();
        for (const auto& [_, barrier] : stageBarriers) // NOLINT (nondeterministic order)
            barrierVector.push_back(barrier);
        stageBarriers.clear();
        cmdbuf->pipelineBarrier2KHR({
            .imageMemoryBarrierCount = static_cast<uint32_t>(barrierVector.size()),
            .pImageMemoryBarriers = barrierVector.data()
        }, dld);

        for (const auto& substage : stage.substages) {
            // Bind shader pipeline for this stage
            const auto& pipeline = this->m_pipelines.at(substage.pipeline);
            cmdbuf->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline, dld);

            // Dispatch all subiterations for this stage
            for (const auto& subiteration : substage.subiterations) {
                const PushConstants pushConstants{
                    .specialFlag = subiteration.isSpecial ? 1U : 0U,
                    .subiteration = subiteration.iterationIndex
                };
                cmdbuf->pushConstants(
                    *this->m_layout.pipelineLayout,
                    vk::ShaderStageFlagBits::eCompute,
                    0,
                    sizeof(PushConstants),
                    &pushConstants,
                    dld
                );

                const auto& dispatch{subiteration.dispatch};
                cmdbuf->dispatch(dispatch.width, dispatch.height, 1, dld);
            }
        }

        // Append barriers for next stage
        for (const auto& sampledImage : stage.sampledImages) {
            const auto& image = this->m_images.at(sampledImage);
            for (const auto& subimage : image.subimages) {
                stageBarriers[*subimage.image] = {
                    .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                    .srcAccessMask = vk::AccessFlagBits2::eShaderRead,
                    .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                    .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                    .image = *subimage.image,
                    .subresourceRange = {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .levelCount = 1,
                        .layerCount = image.subimages.size() == 1 ? image.signature.count : 1
                    }
                };
            }
        }
        for (const auto& storageImage : stage.storageImages) {
            const auto& image = this->m_images.at(storageImage);
            for (const auto& subimage : image.subimages) {
                stageBarriers[*subimage.image] = {
                    .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                    .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
                    .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                    .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                    .image = *subimage.image,
                    .subresourceRange = {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .levelCount = 1,
                        .layerCount = image.subimages.size() == 1 ? image.signature.count : 1
                    }
                };
            }
        }

        // Skip barriers on switch between passes
        if (i + 1 == currentStageBound) {
            stageBarriers.clear();
        }
    }

    for (auto& cmdbuf : this->m_cmdbufs) {
        cmdbuf->end(dld);
    }
}

vk::CommandBuffer Pipeline::buildTransCmdbuf(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    uint32_t iteration,
    uint32_t index,
    uint32_t total
) {
    const bool persist{total > 8};
    const uint64_t key{persist ? ((static_cast<uint64_t>(index) << 32) | total) : index};

    if (persist && this->m_transCmdbufs.contains(key))
        return *this->m_transCmdbufs.at(key);

    auto& cmdbuf{this->m_transCmdbufs[key]};
    cmdbuf = vkhelper::createCommandBuffer(
        dld,
        device,
        *this->m_pool
    );

    cmdbuf->begin({
        .flags = persist ? vk::CommandBufferUsageFlagBits::eSimultaneousUse :
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    }, dld);

    const UniformBuffer buf{
        .timestamp = static_cast<float>(index + 1) / static_cast<float>(total + 1),
        .iteration = iteration
    };
    cmdbuf->updateBuffer(
        *this->m_descriptorSet.buffer.first,
        0,
        4,
        static_cast<const void*>(&buf.timestamp),
        dld
    );

    cmdbuf->end(dld);

    return *cmdbuf;
}
