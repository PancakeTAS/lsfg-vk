/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "gamma1.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::backend;

Gamma1::Gamma1(const Ctx& ctx, size_t idx,
        const std::vector<vk::Image>& sourceImages,
        const vk::Image& additionalInput0,
        const vk::Image& additionalInput1) {
    const size_t m = ctx.perf ? 1 : 2; // multiplier
    const VkExtent2D extent = sourceImages.at(0).getExtent();

    // create temporary & output images
    for (size_t i = 0; i < (2 * m); i++) {
        this->tempImages0.emplace_back(ctx.vk, extent);
        this->tempImages1.emplace_back(ctx.vk, extent);
    }
    this->image.emplace(ctx.vk,
        VkExtent2D { extent.width, extent.height },
        VK_FORMAT_R16G16B16A16_SFLOAT
    );

    // create descriptor sets
    const auto& shaders = (ctx.perf ?
        ctx.shaders.get().performance : ctx.shaders.get().quality).gamma;
    this->sets.reserve(4);
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(sourceImages)
        .storages(this->tempImages0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(1)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages0)
        .storages(this->tempImages1)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(2)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages1)
        .storages(this->tempImages0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(3)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages0)
        .sampled(additionalInput0)
        .sampled(additionalInput1)
        .storage(*this->image)
        .sampler(ctx.bnbSampler)
        .sampler(ctx.eabSampler)
        .buffer(ctx.constantBuffers.at(idx))
        .build(ctx.vk, ctx.pool, shaders.at(4)));

    // store dispatch extents
    this->dispatchExtent = backend::add_shift_extent(extent, 7, 3);
}

void Gamma1::prepare(std::vector<VkImage>& images) const {
    for (size_t i = 0; i < this->tempImages0.size(); i++) {
        images.push_back(this->tempImages0.at(i).handle());
        images.push_back(this->tempImages1.at(i).handle());
    }
    images.push_back(this->image->handle());
}

void Gamma1::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const {
    for (const auto& set : this->sets)
        set.dispatch(vk, cmd, dispatchExtent);
}
