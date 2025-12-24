/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "beta0.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::backend;

Beta0::Beta0(const Ctx& ctx,
        const std::vector<std::vector<vk::Image>>& sourceImages) {
    const VkExtent2D extent = sourceImages.at(0).at(0).getExtent();

    // create output images
    this->images.reserve(2);
    for(size_t i = 0; i < 2; i++)
        this->images.emplace_back(ctx.vk, extent);

    // create descriptor sets
    const auto& shader = (ctx.perf ?
        ctx.shaders.get().performance : ctx.shaders.get().quality).beta.at(0);
    this->sets.reserve(sourceImages.size());
    for (size_t i = 0; i < sourceImages.size(); i++)
        this->sets.emplace_back(ManagedShaderBuilder()
            .sampleds(sourceImages.at((i + (sourceImages.size() - 2)) % sourceImages.size()))
            .sampleds(sourceImages.at((i + (sourceImages.size() - 1)) % sourceImages.size()))
            .sampleds(sourceImages.at(i % sourceImages.size()))
            .storages(this->images)
            .sampler(ctx.bnwSampler)
            .build(ctx.vk, ctx.pool, shader));

    // store dispatch extents
    this->dispatchExtent = backend::add_shift_extent(extent, 7, 3);
}

void Beta0::prepare(std::vector<VkImage>& images) const {
    for (const auto& img : this->images)
        images.push_back(img.handle());
}

void Beta0::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const {
    this->sets[idx % this->sets.size()].dispatch(vk, cmd, dispatchExtent);
}
