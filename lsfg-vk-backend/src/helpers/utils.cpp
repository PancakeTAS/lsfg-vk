#include "utils.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

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

std::string ls::to_hex_id(uint32_t id) {
    const std::array<char, 17> chars = std::to_array("0123456789ABCDEF");

    std::string result = "0x";
    result += chars.at((id >> 12) & 0xF);
    result += chars.at((id >> 8) & 0xF);
    result += chars.at((id >> 4) & 0xF);
    result += chars.at(id & 0xF);
    return result;
}
