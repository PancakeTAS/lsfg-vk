#include "beta0.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace chains;

Beta0::Beta0(const ls::Ctx& ctx,
        const std::vector<std::vector<vk::Image>>& sourceImages) {
    const VkExtent2D extent = sourceImages.at(0).at(0).getExtent();

    // create output images
    this->images.reserve(2);
    for(size_t i = 0; i < 2; i++)
        this->images.emplace_back(ctx.vk, extent);

    // create descriptor sets
    const auto& shader = (ctx.perf ?
        ctx.shaders.get().performance : ctx.shaders.get().quality).beta.at(0);
    this->sets.reserve(3);
    for (size_t i = 0; i < 3; i++)
        this->sets.emplace_back(ls::ManagedShaderBuilder()
            .sampleds(sourceImages.at((i + 1) % 3))
            .sampleds(sourceImages.at((i + 2) % 3))
            .sampleds(sourceImages.at(i % 3))
            .storages(this->images)
            .sampler(ctx.bnwSampler)
            .build(ctx.vk, shader));

    // store dispatch extents
    this->dispatchExtent = ls::add_shift_extent(extent, 7, 3);
}

void Beta0::prepare(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const {
    for (const auto& img : this->images)
        cmd.prepareImage(vk, img);
}

void Beta0::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd, size_t idx) const {
    this->sets[idx % 3].dispatch(vk, cmd, dispatchExtent);
}
