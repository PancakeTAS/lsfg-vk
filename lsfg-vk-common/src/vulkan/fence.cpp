#include "lsfg-vk-common/vulkan/fence.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstdint>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a fence
    ls::owned_ptr<VkFence> createFence(const vk::Vulkan& vk) {
        VkFence handle{};

        const VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };
        auto res = vk.df().CreateFence(vk.dev(), &fenceInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateFence() failed");

        return ls::owned_ptr<VkFence>(
            new VkFence(handle),
            [dev = vk.dev(), defunc = vk.df().DestroyFence](VkFence& fence) {
                defunc(dev, fence, nullptr);
            }
        );
    }
}

Fence::Fence(const vk::Vulkan& vk)
    : fence(createFence(vk)) {}

void Fence::reset(const vk::Vulkan& vk) const {
    VkFence fence = *this->fence;
    auto res = vk.df().ResetFences(vk.dev(), 1, &fence);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkResetFences() failed");
}

bool Fence::wait(const vk::Vulkan& vk, uint64_t timeout) const {
    VkFence fence = *this->fence;
    auto res = vk.df().WaitForFences(vk.dev(), 1, &fence, VK_TRUE, timeout);
    if (res != VK_SUCCESS && res != VK_TIMEOUT)
        throw ls::vulkan_error(res, "vkWaitForFences() failed");

    return res == VK_SUCCESS;
}
