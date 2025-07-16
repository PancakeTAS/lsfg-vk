#include "core/descriptorpool.hpp"
#include "core/device.hpp"
#include "common/exception.hpp"

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace LSFG::Core;

DescriptorPool::DescriptorPool(const Core::Device& device) {
    // create descriptor pool
    const std::array<VkDescriptorPoolSize, 4> pools{{ // arbitrary limits
        { .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 4096 },
        { .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 4096 },
        { .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 4096 },
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 4096 }
    }};
    const VkDescriptorPoolCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 16384,
        .poolSizeCount = static_cast<uint32_t>(pools.size()),
        .pPoolSizes = pools.data()
    };
    VkDescriptorPool poolHandle{};
    auto res = vkCreateDescriptorPool(device.handle(), &desc, nullptr, &poolHandle);
    if (res != VK_SUCCESS || poolHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Unable to create descriptor pool");

    // store pool in shared ptr
    this->descriptorPool = std::shared_ptr<VkDescriptorPool>(
        new VkDescriptorPool(poolHandle),
        [dev = device.handle()](VkDescriptorPool* poolHandle) {
            vkDestroyDescriptorPool(dev, *poolHandle, nullptr);
        }
    );
}
