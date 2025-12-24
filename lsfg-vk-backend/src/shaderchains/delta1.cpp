/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "delta1.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::backend;

Delta1::Delta1(const Ctx& ctx, size_t idx,
        const std::vector<vk::Image>& sourceImages0,
        const std::vector<vk::Image>& sourceImages1,
        const vk::Image& additionalInput0,
        const vk::Image& additionalInput1,
        const vk::Image& additionalInput2) {
    const size_t m = ctx.perf ? 1 : 2; // multiplier
    const VkExtent2D extent = sourceImages0.at(0).getExtent();

    // create temporary & output images
    for (size_t i = 0; i < (2 * m); i++) {
        this->tempImages0.emplace_back(ctx.vk, extent);
        this->tempImages1.emplace_back(ctx.vk, extent);
    }
    this->image0.emplace(ctx.vk,
        VkExtent2D { extent.width, extent.height },
        VK_FORMAT_R16G16B16A16_SFLOAT
    );
    this->image1.emplace(ctx.vk,
        VkExtent2D { extent.width, extent.height },
        VK_FORMAT_R16G16B16A16_SFLOAT
    );

    // create descriptor sets
    const auto& shaders = (ctx.perf ?
        ctx.shaders.get().performance : ctx.shaders.get().quality).delta;
    this->sets.reserve(4 + 4);

    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(sourceImages0)
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
        .storage(*this->image0)
        .sampler(ctx.bnbSampler)
        .sampler(ctx.eabSampler)
        .buffer(ctx.constantBuffers.at(idx))
        .build(ctx.vk, ctx.pool, shaders.at(4)));

    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(sourceImages1)
        .storages(this->tempImages0, 0, m)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(6)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages0, 0, m)
        .storages(this->tempImages1, 0, m)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(7)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages1, 0, m)
        .storages(this->tempImages0, 0, m)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(8)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages0, 0, m)
        .sampled(additionalInput2)
        .storage(*this->image1)
        .sampler(ctx.bnbSampler)
        .sampler(ctx.eabSampler)
        .buffer(ctx.constantBuffers.at(idx))
        .build(ctx.vk, ctx.pool, shaders.at(9)));

    // store dispatch extents
    this->dispatchExtent = backend::add_shift_extent(extent, 7, 3);
}

void Delta1::prepare(std::vector<VkImage>& images) const {
    for (size_t i = 0; i < this->tempImages0.size(); i++) {
        images.push_back(this->tempImages0.at(i).handle());
        images.push_back(this->tempImages1.at(i).handle());
    }
    images.push_back(this->image0->handle());
    images.push_back(this->image1->handle());
}

void Delta1::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const {
    for (const auto& set : this->sets)
        set.dispatch(vk, cmd, dispatchExtent);
}
