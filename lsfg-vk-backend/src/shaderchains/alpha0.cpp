#include "alpha0.hpp"
#include "../helpers/utils.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"

#include <cstddef>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace chains;

Alpha0::Alpha0(const ls::Ctx& ctx,
        const vk::Image& sourceImage) {
    const size_t m = ctx.perf ? 1 : 2; // multiplier
    const VkExtent2D halfExtent = ls::add_shift_extent(sourceImage.getExtent(), 1, 1);
    const VkExtent2D quarterExtent = ls::add_shift_extent(halfExtent, 1, 1);

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
    this->sets.emplace_back(ls::ManagedShaderBuilder()
        .sampled(sourceImage)
        .storages(this->tempImages0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, shaders.alpha.at(0)));
    this->sets.emplace_back(ls::ManagedShaderBuilder()
        .sampleds(this->tempImages0)
        .storages(this->tempImages1)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, shaders.alpha.at(1)));
    this->sets.emplace_back(ls::ManagedShaderBuilder()
        .sampleds(this->tempImages1)
        .storages(this->images)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, shaders.alpha.at(2)));

    // store dispatch extents
    this->dispatchExtent0 = ls::add_shift_extent(halfExtent, 7, 3);
    this->dispatchExtent1 = ls::add_shift_extent(quarterExtent, 7, 3);
}

void Alpha0::prepare(const vk::CommandBuffer& cmd) const {
    // TODO: find a way to batch prepare

    for (size_t i = 0; i < this->tempImages0.size(); i++) {
        cmd.prepareImage(this->tempImages0.at(i));
        cmd.prepareImage(this->tempImages1.at(i));
    }

    for (const auto& image : this->images)
        cmd.prepareImage(image);
}

void Alpha0::render(const vk::CommandBuffer& cmd) const {
    this->sets[0].dispatch(cmd, this->dispatchExtent0);
    this->sets[1].dispatch(cmd, this->dispatchExtent0);
    this->sets[2].dispatch(cmd, this->dispatchExtent1);
}
