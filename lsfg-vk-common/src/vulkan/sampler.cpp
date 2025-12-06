#include "lsfg-vk-common/vulkan/sampler.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a sampler
    ls::owned_ptr<VkSampler> createSampler(const vk::Vulkan& vk,
            VkSamplerAddressMode mode, VkCompareOp compare, bool white) {
        VkSampler handle{};

        const VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = mode,
            .addressModeV = mode,
            .addressModeW = mode,
            .compareOp = compare,
            .maxLod = VK_LOD_CLAMP_NONE,
            .borderColor =
                white ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
                    : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
        };
        auto res = vk.df().CreateSampler(vk.dev(), &samplerInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateSampler() failed");

        return ls::owned_ptr<VkSampler>(
            new VkSampler(handle),
            [dev = vk.dev(), defunc = vk.df().DestroySampler](VkSampler& sampler) {
                defunc(dev, sampler, nullptr);
            }
        );
    }
}

Sampler::Sampler(const vk::Vulkan& vk, VkSamplerAddressMode mode, VkCompareOp compare, bool white)
    : sampler(createSampler(vk, mode, compare, white)) {}
