/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "generate.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::backend;

Generate::Generate(const Ctx& ctx, size_t idx,
        const std::pair<vk::Image, vk::Image>& sourceImages,
        const vk::Image& inputImage1,
        const vk::Image& inputImage2,
        const vk::Image& inputImage3,
        const vk::Image& outputImage) {
    // create descriptor sets
    const auto& shader = ctx.hdr ?
        ctx.shaders.get().generate_hdr : ctx.shaders.get().generate;
    this->sets.reserve(2);
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampled(sourceImages.second)
        .sampled(sourceImages.first)
        .sampled(inputImage1)
        .sampled(inputImage2)
        .sampled(inputImage3)
        .storage(outputImage)
        .sampler(ctx.bnbSampler)
        .sampler(ctx.eabSampler)
        .buffer(ctx.constantBuffers.at(idx))
        .build(ctx.vk, ctx.pool, shader));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampled(sourceImages.first)
        .sampled(sourceImages.second)
        .sampled(inputImage1)
        .sampled(inputImage2)
        .sampled(inputImage3)
        .storage(outputImage)
        .sampler(ctx.bnbSampler)
        .sampler(ctx.eabSampler)
        .buffer(ctx.constantBuffers.at(idx))
        .build(ctx.vk, ctx.pool, shader));

    // store dispatch extent
    this->dispatchExtent = backend::add_shift_extent(ctx.sourceExtent, 15, 4);
}

void Generate::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const {
    this->sets[idx % 2].dispatch(vk, cmd, this->dispatchExtent);
}
