#include "utils.hpp"

#include <cstddef>
#include <cstdint>

#include <vulkan/vulkan_core.h>

using namespace ls;

ConstantBuffer ls::getDefaultConstantBuffer(
        size_t index, size_t total,
        bool hdr, float invFlow) {
    return ConstantBuffer {
        .advancedColorKind = hdr ? 2U : 0U,
        .hdrSupport = hdr ? 1U : 0U,
        .resolutionInvScale = invFlow,
        .timestamp = static_cast<float>(index + 1) / static_cast<float>(total + 1),
        .uiThreshold = 0.5F
    };
}

VkExtent2D ls::shift_extent(VkExtent2D extent, uint32_t i) {
    return VkExtent2D{
        .width = extent.width >> i,
        .height = extent.height >> i
    };
}

VkExtent2D ls::add_shift_extent(VkExtent2D extent, uint32_t a, uint32_t i) {
    return VkExtent2D{
        .width = (extent.width + a) >> i,
        .height = (extent.height + a) >> i
    };
}
