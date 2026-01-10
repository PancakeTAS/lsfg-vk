/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "device.hpp"
#include "instance.hpp"
#include "layer.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk;
using namespace lsfgvk::layer;

namespace {
    /// helper function to add required extensions
    std::vector<const char*> add_extensions(const char* const* existingExtensions, size_t count,
            const std::vector<const char*>& requiredExtensions) {
        std::vector<const char*> extensions(count);
        std::copy_n(existingExtensions, count, extensions.data());

        for (const auto& requiredExtension : requiredExtensions) {
            auto it = std::ranges::find_if(extensions,
                [requiredExtension](const char* extension) {
                    return std::string(extension) == std::string(requiredExtension);
                });
            if (it == extensions.end())
                extensions.push_back(requiredExtension);
        }

        return extensions;
    }
    /// helper function for finding a graphics queue family
    uint32_t find_qfi(VkPhysicalDevice physdev, const vk::VulkanInstanceFuncs& funcs) {
        uint32_t queueFamilyCount = 0;
        funcs.GetPhysicalDeviceQueueFamilyProperties(physdev, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        funcs.GetPhysicalDeviceQueueFamilyProperties(physdev, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                return i;

        throw ls::error("no graphics queue family found");
    }
}

MyVkDevice::MyVkDevice(MyVkLayer& layer, MyVkInstance& instance,
            VkPhysicalDevice physdev, VkDeviceCreateInfo info,
            PFN_vkGetDeviceProcAddr addr, PFN_vkSetDeviceLoaderData loader_addr,
            const std::function<VkDevice(VkDeviceCreateInfo*)>& createFunc) :
        layer(std::ref(layer)), instance(std::ref(instance)) {
    // add required extensions
    auto extensions = add_extensions(
        info.ppEnabledExtensionNames,
        info.enabledExtensionCount,
        {
            "VK_KHR_swapchain",
            "VK_KHR_external_memory",
            "VK_KHR_external_memory_fd",
            "VK_KHR_external_semaphore",
            "VK_KHR_external_semaphore_fd",
            "VK_KHR_timeline_semaphore"
        }
    );
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();

    // enable timeline semaphores
    bool isFeatureEnabled = false;
    auto* featureInfo = reinterpret_cast<VkBaseInStructure*>(const_cast<void*>(info.pNext));
    while (featureInfo) {
        if (featureInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES) {
            auto* features = reinterpret_cast<VkPhysicalDeviceVulkan12Features*>(featureInfo);
            features->timelineSemaphore = VK_TRUE;
            isFeatureEnabled = true;
        } else if (featureInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES) {
            auto* features = reinterpret_cast<VkPhysicalDeviceTimelineSemaphoreFeatures*>(featureInfo);
            features->timelineSemaphore = VK_TRUE;
            isFeatureEnabled = true;
        }

        featureInfo = const_cast<VkBaseInStructure*>(featureInfo->pNext);
    }

    VkPhysicalDeviceTimelineSemaphoreFeatures timelineFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
        .pNext = const_cast<void*>(info.pNext),
        .timelineSemaphore = VK_TRUE
    };
    if (!isFeatureEnabled)
        info.pNext = &timelineFeatures;

    // append a graphics queue
    std::vector<VkDeviceQueueCreateInfo> queues;
    queues.reserve(info.queueCreateInfoCount + 1);
    for (uint32_t i = 0; i < info.queueCreateInfoCount; ++i)
        queues.push_back(info.pQueueCreateInfos[i]);

    const uint32_t qfi = find_qfi(physdev, instance.funcs());
    std::optional<uint32_t> queueIdx{0};
    for (auto& queueInfo : queues) {
        if (queueInfo.queueFamilyIndex == qfi) {
            queueIdx.emplace(queueInfo.queueCount);
            queueInfo.queueCount++; // we pray it doesn't exceed the max
            break;
        }
    }

    if (!queueIdx.has_value()) {
        const VkDeviceQueueCreateInfo queueInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = qfi,
            .queueCount = 2,
            .pQueuePriorities = new float[1]{ 1.0F }
        };
        queues.push_back(queueInfo);
    }

    info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    info.pQueueCreateInfos = queues.data();

    // create device
    this->handle = createFunc(&info);
    this->dfuncs = vk::initVulkanDeviceFuncs(addr, this->handle, true);
    this->vk.emplace(vk::Vulkan(
        instance.instance(), this->handle, physdev,
        instance.funcs(), this->dfuncs,
        true,
        loader_addr
    ));

    // extract the graphics queues
    this->dfuncs.GetDeviceQueue(this->handle, qfi, queueIdx.value_or(0), &this->offloadQueue.queue);
    loader_addr(this->handle, this->offloadQueue.queue);
}
