#include "core/commandpool.hpp"
#include "core/device.hpp"
#include "common/exception.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

using namespace LSFG::Core;

CommandPool::CommandPool(const Core::Device& device) {
    // create command pool
    const VkCommandPoolCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = device.getComputeFamilyIdx()
    };
    VkCommandPool commandPoolHandle{};
    auto res = vkCreateCommandPool(device.handle(), &desc, nullptr, &commandPoolHandle);
    if (res != VK_SUCCESS || commandPoolHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Unable to create command pool");

    // store command pool in shared ptr
    this->commandPool = std::shared_ptr<VkCommandPool>(
        new VkCommandPool(commandPoolHandle),
        [dev = device.handle()](VkCommandPool* commandPoolHandle) {
            vkDestroyCommandPool(dev, *commandPoolHandle, nullptr);
        }
    );
}
