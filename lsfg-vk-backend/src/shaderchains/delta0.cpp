/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "delta0.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::backend;

Delta0::Delta0(const Ctx& ctx, size_t idx,
        const std::vector<std::vector<vk::Image>>& sourceImages,
        const vk::Image& additionalInput0,
        const vk::Image& additionalInput1) {
    const size_t m = ctx.perf ? 1 : 2; // multiplier
    const VkExtent2D extent = sourceImages.at(0).at(0).getExtent();

    // create output images
    this->images0.reserve(3);
    for(size_t i = 0; i < 3; i++)
        this->images0.emplace_back(ctx.vk, extent);
    this->images1.reserve(m);
    for (size_t i = 0; i < m; i++)
        this->images1.emplace_back(ctx.vk, extent);

    // create descriptor sets
    const auto& shaders = (ctx.perf ?
        ctx.shaders.get().performance : ctx.shaders.get().quality).delta;

    this->sets0.reserve(sourceImages.size());
    for (size_t i = 0; i < sourceImages.size(); i++)
        this->sets0.emplace_back(ManagedShaderBuilder()
            .sampleds(sourceImages.at((i + (sourceImages.size() - 1)) % sourceImages.size()))
            .sampleds(sourceImages.at(i % sourceImages.size()))
            .sampled(additionalInput0)
            .storages(this->images0)
            .sampler(ctx.bnwSampler)
            .sampler(ctx.eabSampler)
            .buffer(ctx.constantBuffers.at(idx))
            .build(ctx.vk, ctx.pool, shaders.at(0)));

    this->sets1.reserve(sourceImages.size());
    for (size_t i = 0; i < sourceImages.size(); i++)
        this->sets1.emplace_back(ManagedShaderBuilder()
            .sampleds(sourceImages.at((i + (sourceImages.size() - 1)) % sourceImages.size()))
            .sampleds(sourceImages.at(i % sourceImages.size()))
            .sampled(additionalInput1)
            .sampled(additionalInput0)
            .storages(this->images1)
            .sampler(ctx.bnwSampler)
            .sampler(ctx.eabSampler)
            .buffer(ctx.constantBuffers.at(idx))
            .build(ctx.vk, ctx.pool, shaders.at(5)));

    // store dispatch extents
    this->dispatchExtent = backend::add_shift_extent(extent, 7, 3);
}

void Delta0::prepare(std::vector<VkImage>& images) const {
    for (const auto& img : this->images0)
        images.push_back(img.handle());
    for (const auto& img : this->images1)
        images.push_back(img.handle());
}

void Delta0::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const {
    this->sets0[idx % this->sets0.size()].dispatch(vk, cmd, dispatchExtent);
    this->sets1[idx % this->sets1.size()].dispatch(vk, cmd, dispatchExtent);
}
