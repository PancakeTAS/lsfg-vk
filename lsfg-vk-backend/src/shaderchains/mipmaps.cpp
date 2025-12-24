/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "mipmaps.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::backend;

Mipmaps::Mipmaps(const Ctx& ctx,
        const std::pair<vk::Image, vk::Image>& sourceImages) {
    // create output images for base and 6 mips
    this->images.reserve(7);
    for (uint32_t i = 0; i < 7; i++)
       this->images.emplace_back(ctx.vk,
            backend::shift_extent(ctx.flowExtent, i), VK_FORMAT_R8_UNORM);

    // create descriptor sets for both input images
    this->sets.reserve(2);
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampled(sourceImages.first)
        .storages(this->images)
        .sampler(ctx.bnbSampler)
        .buffer(ctx.constantBuffer)
        .build(ctx.vk, ctx.pool, ctx.shaders.get().mipmaps));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampled(sourceImages.second)
        .storages(this->images)
        .sampler(ctx.bnbSampler)
        .buffer(ctx.constantBuffer)
        .build(ctx.vk, ctx.pool, ctx.shaders.get().mipmaps));

    // store dispatch extent
    this->dispatchExtent = backend::add_shift_extent(ctx.flowExtent, 63, 6);
}

void Mipmaps::prepare(std::vector<VkImage>& images) const {
    for (const auto& img : this->images)
        images.push_back(img.handle());
}

void Mipmaps::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const {
    this->sets[idx % 2].dispatch(vk, cmd, this->dispatchExtent);
}
