#include "mipmaps.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace chains;

Mipmaps::Mipmaps(const ls::Ctx& ctx,
        const std::pair<vk::Image, vk::Image>& sourceImages) {
    // create output images for base and 6 mips
    this->images.reserve(7);
    for (uint32_t i = 0; i < 7; i++)
       this->images.emplace_back(ctx.vk,
            ls::shift_extent(ctx.flowExtent, i), VK_FORMAT_R8_UNORM);

    // create descriptor sets for both input images
    this->sets.reserve(2);
    this->sets.emplace_back(ls::ManagedShaderBuilder()
        .sampled(sourceImages.first)
        .storages(this->images)
        .sampler(ctx.bnbSampler)
        .buffer(ctx.constantBuffer)
        .build(ctx.vk, ctx.shaders.get().mipmaps));
    this->sets.emplace_back(ls::ManagedShaderBuilder()
        .sampled(sourceImages.second)
        .storages(this->images)
        .sampler(ctx.bnbSampler)
        .buffer(ctx.constantBuffer)
        .build(ctx.vk, ctx.shaders.get().mipmaps));

    // store dispatch extent
    this->dispatchExtent = ls::add_shift_extent(ctx.flowExtent, 63, 6);
}

void Mipmaps::prepare(const vk::CommandBuffer& cmd) const {
    for (const auto& img : this->images)
        cmd.prepareImage(img);
}

void Mipmaps::render(const vk::CommandBuffer& cmd, size_t idx) const {
    this->sets[idx % 2].dispatch(cmd, this->dispatchExtent);
}
