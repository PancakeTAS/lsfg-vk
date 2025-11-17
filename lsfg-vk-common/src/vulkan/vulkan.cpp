#include "lsfg-vk-common/vulkan/vulkan.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a vulkan instance
    ls::owned_ptr<VkInstance> createInstance(
            const std::string& appName, version appVersion,
            const std::string& engineName, version engineVersion) {
        VkInstance handle{};

        const VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = appName.c_str(),
            .applicationVersion = appVersion.into(),
            .pEngineName = engineName.c_str(),
            .engineVersion = engineVersion.into(),
            .apiVersion = VK_API_VERSION_1_2 // seems Vulkan 1.2 is supported on all Vulkan-capable GPUs
        };
        const VkInstanceCreateInfo instanceInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo
        };
        auto res = vkCreateInstance(&instanceInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateInstance() failed");

        return ls::owned_ptr<VkInstance>(
            new VkInstance(handle),
            [](VkInstance& instance) {
                vkDestroyInstance(instance, nullptr);
            }
        );
    }

    /// filter for a physical device
    VkPhysicalDevice findPhysicalDevice(
            VkInstance instance,
            PhysicalDeviceSelector filter) {
        uint32_t phydevCount{};
        auto res = vkEnumeratePhysicalDevices(instance, &phydevCount, nullptr);
        if (res != VK_SUCCESS || phydevCount == 0)
            throw ls::vulkan_error(res, "vkEnumeratePhysicalDevices() failed");

        std::vector<VkPhysicalDevice> phydevs(phydevCount);
        res = vkEnumeratePhysicalDevices(instance, &phydevCount, phydevs.data());
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkEnumeratePhysicalDevices() failed");

        VkPhysicalDevice selected = filter(phydevs);
        if (!selected)
            throw ls::vulkan_error("no suitable physical device found");

        return selected;
    }

    /// find the queue family index with given flags
    uint32_t findQFI(VkPhysicalDevice physdev, VkQueueFlags flags) {
        uint32_t queueCount{};
        vkGetPhysicalDeviceQueueFamilyProperties(physdev, &queueCount, nullptr);

        std::vector<VkQueueFamilyProperties> queues(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physdev, &queueCount, queues.data());

        for (uint32_t i = 0; i < queueCount; ++i) {
            if ((queues[i].queueFlags & flags) == flags)
                return i;
        }

        throw ls::vulkan_error("no queue family with requested flags found");
    }

    /// check for fp16 support
    bool checkFP16(VkPhysicalDevice physdev) {
        VkPhysicalDeviceVulkan12Features supportedFeaturesVulkan12{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
        };
        VkPhysicalDeviceFeatures2 supportedFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &supportedFeaturesVulkan12
        };
        vkGetPhysicalDeviceFeatures2(physdev, &supportedFeatures);
        return supportedFeaturesVulkan12.shaderFloat16 == VK_TRUE;
    }

    /// create a logical device
    ls::owned_ptr<VkDevice> createLogicalDevice(VkPhysicalDevice physdev, uint32_t cfi, bool fp16) {
        VkDevice handle{};

        const float queuePriority{1.0F}; // highest priority
        const VkPhysicalDeviceVulkan12Features requestedFeaturesVulkan12{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .shaderFloat16 = fp16,
            .timelineSemaphore = VK_TRUE,
            .vulkanMemoryModel = VK_TRUE
        };
        const VkDeviceQueueCreateInfo requestedQueueInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = cfi,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
        const std::vector<const char*> requestedExtensions{
            VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
            VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME // TODO: possibly attempt to get rid of
        };
        const VkDeviceCreateInfo deviceInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &requestedFeaturesVulkan12,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &requestedQueueInfo,
            .enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size()),
            .ppEnabledExtensionNames = requestedExtensions.data()
        };
        auto res = vkCreateDevice(physdev, &deviceInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateDevice() failed");

        return ls::owned_ptr<VkDevice>(
            new VkDevice(handle),
            [](VkDevice& device) {
                vkDestroyDevice(device, nullptr);
            }
        );
    }

    /// get a queue from the logical device
    VkQueue getQueue(VkDevice device, uint32_t cfi) {
        VkQueue queue{};

        vkGetDeviceQueue(device, cfi, 0, &queue);
        return queue;
    }

    /// create a command pool
    ls::owned_ptr<VkCommandPool> createCommandPool(VkDevice device, uint32_t cfi) {
        VkCommandPool handle{};

        const VkCommandPoolCreateInfo cmdpoolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = cfi
        };
        auto res = vkCreateCommandPool(device, &cmdpoolInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateCommandPool() failed");

        return ls::owned_ptr<VkCommandPool>(
            new VkCommandPool(handle),
            [dev = device](VkCommandPool& pool) {
                vkDestroyCommandPool(dev, pool, nullptr);
            }
        );
    }

    /// create a descriptor pool
    ls::owned_ptr<VkDescriptorPool> createDescriptorPool(VkDevice device) {
        VkDescriptorPool handle{};

        const std::array<VkDescriptorPoolSize, 4> poolCounts{{ // FIXME: arbitrary limits
            { .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 4096 },
            { .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 4096 },
            { .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 4096 },
            { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 4096 }
        }};
        const VkDescriptorPoolCreateInfo descpoolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 16384,
            .poolSizeCount = static_cast<uint32_t>(poolCounts.size()),
            .pPoolSizes = poolCounts.data()
        };
        auto res = vkCreateDescriptorPool(device, &descpoolInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateDescriptorPool() failed");

        return ls::owned_ptr<VkDescriptorPool>(
            new VkDescriptorPool(handle),
            [dev = device](VkDescriptorPool& pool) {
                vkDestroyDescriptorPool(dev, pool, nullptr);
            }
        );
    }
}

Vulkan::Vulkan(const std::string& appName, version appVersion,
        const std::string& engineName, version engineVersion,
        PhysicalDeviceSelector selectPhysicalDevice) :
    instance(createInstance(
        appName, appVersion,
        engineName, engineVersion
    )),
    physdev(findPhysicalDevice(
        *this->instance,
        selectPhysicalDevice
    )),
    computeFamilyIdx(findQFI(this->physdev, VK_QUEUE_COMPUTE_BIT)),
    fp16(checkFP16(this->physdev)),
    device(createLogicalDevice(
        this->physdev,
        this->computeFamilyIdx,
        this->fp16
    )),
    computeQueue(getQueue(*this->device, this->computeFamilyIdx)),
    cmdPool(createCommandPool(
        *this->device,
        this->computeFamilyIdx
    )),
    descPool(createDescriptorPool(
        *this->device
    )) {
}

std::optional<uint32_t> Vulkan::findMemoryTypeIndex(
        std::bitset<32> validTypes, bool hostVisibility) const {
    const VkMemoryPropertyFlags desiredProps = hostVisibility ?
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) :
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(this->physdev, &props);

    std::array<VkMemoryType, 32> memTypes = std::to_array(props.memoryTypes);
    for (uint32_t i = 0; i < props.memoryTypeCount; ++i)
        if (validTypes.test(i) && (memTypes.at(i).propertyFlags & desiredProps) == desiredProps)
            return i;

    return std::nullopt;
}
