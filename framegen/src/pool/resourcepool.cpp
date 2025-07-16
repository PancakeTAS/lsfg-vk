#include "pool/resourcepool.hpp"
#include "core/buffer.hpp"
#include "core/device.hpp"
#include "core/sampler.hpp"

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>

using namespace LSFG;
using namespace LSFG::Pool;

struct ConstantBuffer {
    std::array<uint32_t, 2> inputOffset;
    uint32_t firstIter;
    uint32_t firstIterS;
    uint32_t advancedColorKind;
    uint32_t hdrSupport;
    float resolutionInvScale;
    float timestamp;
    float uiThreshold;
    std::array<uint32_t, 3> pad;
};

Core::Buffer ResourcePool::getBuffer(
            const Core::Device& device,
            float timestamp, bool firstIter, bool firstIterS) {
    uint64_t hash = 0;
    const union { float f; uint32_t i; } u{
        .f = timestamp };
    hash |= u.i;
    hash |= static_cast<uint64_t>(firstIter) << 32;
    hash |= static_cast<uint64_t>(firstIterS) << 33;

    auto it = buffers.find(hash);
    if (it != buffers.end())
        return it->second;

    // create the buffer
    const ConstantBuffer data{
        .inputOffset = { 0, 0 },
        .advancedColorKind = this->isHdr ? 2U : 0U,
        .hdrSupport = this->isHdr,
        .resolutionInvScale = this->flowScale,
        .timestamp = timestamp,
        .uiThreshold = 0.5F,
    };
    Core::Buffer buffer(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    buffers[hash] = buffer;
    return buffer;
}

Core::Sampler ResourcePool::getSampler(
            const Core::Device& device,
            VkSamplerAddressMode type,
            VkCompareOp compare,
            bool isWhite) {
    uint64_t hash = 0;
    hash |= static_cast<uint64_t>(type) << 0;
    hash |= static_cast<uint64_t>(compare) << 8;
    hash |= static_cast<uint64_t>(isWhite) << 16;

    auto it = samplers.find(hash);
    if (it != samplers.end())
        return it->second;

    // create the sampler
    Core::Sampler sampler(device, type, compare, isWhite);
    samplers[hash] = sampler;
    return sampler;
}
