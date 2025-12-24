/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "alpha1.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::backend;

Alpha1::Alpha1(const Ctx& ctx, size_t temporal,
        const std::vector<vk::Image>& sourceImages) {
    const size_t m = ctx.perf ? 1 : 2; // multiplier
    const VkExtent2D quarterExtent = sourceImages.at(0).getExtent();

    // create output images for mod3
    this->images.reserve(temporal);
    for(size_t i = 0; i < temporal; i++) {
        auto& vec = this->images.emplace_back();

        vec.reserve(2 * m);
        for (size_t j = 0; j < (2 * m); j++)
            vec.emplace_back(ctx.vk, quarterExtent);
    }

    // create descriptor sets
    const auto& shaders = ctx.perf ? ctx.shaders.get().performance : ctx.shaders.get().quality;
    this->sets.reserve(temporal);
    for (size_t i = 0; i < temporal; i++)
        this->sets.emplace_back(ManagedShaderBuilder()
            .sampleds(sourceImages)
            .storages(this->images.at(i))
            .sampler(ctx.bnbSampler)
            .build(ctx.vk, ctx.pool, shaders.alpha.at(3)));

    // store dispatch extents
    this->dispatchExtent = backend::add_shift_extent(quarterExtent, 7, 3);
}

void Alpha1::prepare(std::vector<VkImage>& images) const {
    for (const auto& vec : this->images)
        for (const auto& img : vec)
            images.push_back(img.handle());
}

void Alpha1::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const {
    this->sets[idx % this->sets.size()].dispatch(vk, cmd, dispatchExtent);
}
