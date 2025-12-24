/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "alpha0.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::backend;

Alpha0::Alpha0(const Ctx& ctx,
        const vk::Image& sourceImage) {
    const size_t m = ctx.perf ? 1 : 2; // multiplier
    const VkExtent2D halfExtent = backend::add_shift_extent(sourceImage.getExtent(), 1, 1);
    const VkExtent2D quarterExtent = backend::add_shift_extent(halfExtent, 1, 1);

    // create temporary & output images
    this->tempImages0.reserve(m);
    this->tempImages1.reserve(m);
    for (size_t i = 0; i < m; i++) {
        this->tempImages0.emplace_back(ctx.vk, halfExtent);
        this->tempImages1.emplace_back(ctx.vk, halfExtent);
    }

    this->images.reserve(2 * m);
    for (size_t i = 0; i < (2 * m); i++)
        this->images.emplace_back(ctx.vk, quarterExtent);

    // create descriptor sets
    const auto& shaders = ctx.perf ? ctx.shaders.get().performance : ctx.shaders.get().quality;
    this->sets.reserve(3);
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampled(sourceImage)
        .storages(this->tempImages0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.alpha.at(0)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages0)
        .storages(this->tempImages1)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.alpha.at(1)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages1)
        .storages(this->images)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.alpha.at(2)));

    // store dispatch extents
    this->dispatchExtent0 = backend::add_shift_extent(halfExtent, 7, 3);
    this->dispatchExtent1 = backend::add_shift_extent(quarterExtent, 7, 3);
}

void Alpha0::prepare(std::vector<VkImage>& images) const {
    for (size_t i = 0; i < this->tempImages0.size(); i++) {
        images.push_back(this->tempImages0.at(i).handle());
        images.push_back(this->tempImages1.at(i).handle());
    }

    for (const auto& image : this->images)
        images.push_back(image.handle());
}

void Alpha0::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const {
    this->sets[0].dispatch(vk, cmd, this->dispatchExtent0);
    this->sets[1].dispatch(vk, cmd, this->dispatchExtent0);
    this->sets[2].dispatch(vk, cmd, this->dispatchExtent1);
}
