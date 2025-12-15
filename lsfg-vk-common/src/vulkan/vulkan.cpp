#include "lsfg-vk-common/vulkan/vulkan.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <dlfcn.h>
#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// load libvulkan.so.1 and return its handle
    void* get_vulkan_handle() {
        static void* handle{nullptr}; // NOLINT
        if (handle) return handle;

        handle = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
        if (!handle) handle = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        if (!handle)
            throw ls::vulkan_error("failed to load libvulkan.so.1");

        return handle;
    }

    /// get the main proc addr function
    PFN_vkGetInstanceProcAddr get_mpa() {
        static PFN_vkGetInstanceProcAddr mpa{nullptr}; // NOLINT
        if (mpa) return mpa;

        mpa = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            dlsym(get_vulkan_handle(), "vkGetInstanceProcAddr"));
        if (!mpa)
            throw ls::vulkan_error("failed to get vkGetInstanceProcAddr symbol");

        return mpa;
    }
}

namespace {
    template<typename T>
    T ipa(PFN_vkGetInstanceProcAddr mpa, VkInstance instance, const char* name) {
        T func = reinterpret_cast<T>(
            mpa(instance, name));
        if (!func)
            throw ls::vulkan_error("failed to get instance proc addr for " + std::string(name));
        return func;
    }

    /// create a vulkan instance
    ls::owned_ptr<VkInstance> createInstance(
            const std::string& appName, version appVersion,
            const std::string& engineName, version engineVersion) {
        VkInstance handle{};

        auto vkCreateInstance =
            ipa<PFN_vkCreateInstance>(get_mpa(), nullptr, "vkCreateInstance");
        if (!vkCreateInstance)
            throw ls::vulkan_error("failed to get vkCreateInstance symbol");

        const VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = appName.c_str(),
            .applicationVersion = appVersion.into(),
            .pEngineName = engineName.c_str(),
            .engineVersion = engineVersion.into(),
            .apiVersion = VK_API_VERSION_1_2 // seems 1.2 is supported on all Vulkan-capable GPUs
        };
        const VkInstanceCreateInfo instanceInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo
        };
        auto res = vkCreateInstance(&instanceInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateInstance() failed");

        auto defunc =
            ipa<PFN_vkDestroyInstance>(get_mpa(), handle, "vkDestroyInstance");
        if (!defunc)
            throw ls::vulkan_error("failed to get vkDestroyInstance symbol");
        return ls::owned_ptr<VkInstance>(
            new VkInstance(handle),
            [defunc](VkInstance& instance) {
                defunc(instance, nullptr);
            }
        );
    }

    /// filter for a physical device
    VkPhysicalDevice findPhysicalDevice(const VulkanInstanceFuncs& fi,
            VkInstance instance,
            PhysicalDeviceSelector filter) {
        uint32_t phydevCount{};
        auto res = fi.EnumeratePhysicalDevices(instance, &phydevCount, nullptr);
        if (res != VK_SUCCESS || phydevCount == 0)
            throw ls::vulkan_error(res, "vkEnumeratePhysicalDevices() failed");

        std::vector<VkPhysicalDevice> phydevs(phydevCount);
        res = fi.EnumeratePhysicalDevices(instance, &phydevCount, phydevs.data());
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkEnumeratePhysicalDevices() failed");

        VkPhysicalDevice selected = filter(fi, phydevs);
        if (!selected)
            throw ls::vulkan_error("no suitable physical device found");

        return selected;
    }

    /// find the queue family index with given flags
    uint32_t findQFI(const VulkanInstanceFuncs& fi,
            VkPhysicalDevice physdev, VkQueueFlags flags) {
        uint32_t queueCount{};
        fi.GetPhysicalDeviceQueueFamilyProperties(physdev, &queueCount, nullptr);

        std::vector<VkQueueFamilyProperties> queues(queueCount);
        fi.GetPhysicalDeviceQueueFamilyProperties(physdev, &queueCount, queues.data());

        for (uint32_t i = 0; i < queueCount; ++i) {
            if ((queues[i].queueFlags & flags) == flags)
                return i;
        }

        throw ls::vulkan_error("no queue family with requested flags found");
    }

    /// check for fp16 support
    bool checkFP16(const VulkanInstanceFuncs& fi, VkPhysicalDevice physdev) {
        VkPhysicalDeviceVulkan12Features supportedFeaturesVulkan12{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
        };
        VkPhysicalDeviceFeatures2 supportedFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &supportedFeaturesVulkan12
        };
        fi.GetPhysicalDeviceFeatures2(physdev, &supportedFeatures);
        return supportedFeaturesVulkan12.shaderFloat16 == VK_TRUE;
    }

    template<typename T>
    T dpa(const VulkanInstanceFuncs& funcs, VkDevice device, const char* name) {
        T func = reinterpret_cast<T>(
            funcs.GetDeviceProcAddr(device, name));
        if (!func)
            throw ls::vulkan_error("failed to get device proc addr for " + std::string(name));
        return func;
    }

    /// create a logical device
    ls::owned_ptr<VkDevice> createLogicalDevice(const VulkanInstanceFuncs& fi,
            VkPhysicalDevice physdev, uint32_t cfi, bool fp16) {
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
        auto res = fi.CreateDevice(physdev, &deviceInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateDevice() failed");

        auto defunc =
            dpa<PFN_vkDestroyDevice>(fi, handle, "vkDestroyDevice");
        if (!defunc)
            throw ls::vulkan_error("failed to get vkDestroyDevice symbol");
        return ls::owned_ptr<VkDevice>(
            new VkDevice(handle),
            [defunc](VkDevice& device) {
                defunc(device, nullptr);
            }
        );
    }

    /// get a queue from the logical device
    VkQueue getQueue(const VulkanDeviceFuncs& fd,
            VkDevice device, uint32_t cfi) {
        VkQueue queue{};

        fd.GetDeviceQueue(device, cfi, 0, &queue);
        return queue;
    }

    /// create a command pool
    ls::owned_ptr<VkCommandPool> createCommandPool(const VulkanDeviceFuncs& fd,
            VkDevice device, uint32_t cfi) {
        VkCommandPool handle{};

        const VkCommandPoolCreateInfo cmdpoolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = cfi
        };
        auto res = fd.CreateCommandPool(device, &cmdpoolInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateCommandPool() failed");

        return ls::owned_ptr<VkCommandPool>(
            new VkCommandPool(handle),
            [dev = device, defunc = fd.DestroyCommandPool](VkCommandPool& pool) {
                defunc(dev, pool, nullptr);
            }
        );
    }

    /// create a descriptor pool
    ls::owned_ptr<VkDescriptorPool> createDescriptorPool(const VulkanDeviceFuncs& fd,
            VkDevice device) {
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
        auto res = fd.CreateDescriptorPool(device, &descpoolInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateDescriptorPool() failed");

        return ls::owned_ptr<VkDescriptorPool>(
            new VkDescriptorPool(handle),
            [dev = device, defunc = fd.DestroyDescriptorPool](VkDescriptorPool& pool) {
                defunc(dev, pool, nullptr);
            }
        );
    }
}

/// initialize vulkan instance function pointers
VulkanInstanceFuncs vk::initVulkanInstanceFuncs(VkInstance i, PFN_vkGetInstanceProcAddr mpa) {
    return {
        .DestroyInstance = ipa<PFN_vkDestroyInstance>(mpa, i, "vkDestroyInstance"),
        .EnumeratePhysicalDevices = ipa<PFN_vkEnumeratePhysicalDevices>(mpa, i,
            "vkEnumeratePhysicalDevices"),
        .EnumerateDeviceExtensionProperties = ipa<PFN_vkEnumerateDeviceExtensionProperties>(mpa, i,
            "vkEnumerateDeviceExtensionProperties"),
        .GetPhysicalDeviceProperties2 = ipa<PFN_vkGetPhysicalDeviceProperties2>(mpa, i,
            "vkGetPhysicalDeviceProperties2"),
        .GetPhysicalDeviceQueueFamilyProperties =
            ipa<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(mpa, i,
                "vkGetPhysicalDeviceQueueFamilyProperties"),
        .GetPhysicalDeviceFeatures2 = ipa<PFN_vkGetPhysicalDeviceFeatures2>(mpa, i,
            "vkGetPhysicalDeviceFeatures2"),
        .GetPhysicalDeviceMemoryProperties = ipa<PFN_vkGetPhysicalDeviceMemoryProperties>(mpa, i,
            "vkGetPhysicalDeviceMemoryProperties"),
        .CreateDevice = ipa<PFN_vkCreateDevice>(mpa, i, "vkCreateDevice"),
        .GetDeviceProcAddr = ipa<PFN_vkGetDeviceProcAddr>(mpa, i, "vkGetDeviceProcAddr"),
    };
}

/// initialize vulkan device function pointers
VulkanDeviceFuncs vk::initVulkanDeviceFuncs(const VulkanInstanceFuncs& f, VkDevice d) {
    return {
        .GetDeviceQueue = dpa<PFN_vkGetDeviceQueue>(f, d, "vkGetDeviceQueue"),
        .DeviceWaitIdle = dpa<PFN_vkDeviceWaitIdle>(f, d, "vkDeviceWaitIdle"),
        .CreateCommandPool = dpa<PFN_vkCreateCommandPool>(f, d, "vkCreateCommandPool"),
        .DestroyCommandPool = dpa<PFN_vkDestroyCommandPool>(f, d, "vkDestroyCommandPool"),
        .CreateDescriptorPool = dpa<PFN_vkCreateDescriptorPool>(f, d, "vkCreateDescriptorPool"),
        .DestroyDescriptorPool = dpa<PFN_vkDestroyDescriptorPool>(f, d, "vkDestroyDescriptorPool"),
        .CreateBuffer = dpa<PFN_vkCreateBuffer>(f, d, "vkCreateBuffer"),
        .DestroyBuffer = dpa<PFN_vkDestroyBuffer>(f, d, "vkDestroyBuffer"),
        .GetBufferMemoryRequirements = dpa<PFN_vkGetBufferMemoryRequirements>(f, d,
            "vkGetBufferMemoryRequirements"),
        .AllocateMemory = dpa<PFN_vkAllocateMemory>(f, d, "vkAllocateMemory"),
        .FreeMemory = dpa<PFN_vkFreeMemory>(f, d, "vkFreeMemory"),
        .BindBufferMemory = dpa<PFN_vkBindBufferMemory>(f, d, "vkBindBufferMemory"),
        .MapMemory = dpa<PFN_vkMapMemory>(f, d, "vkMapMemory"),
        .UnmapMemory = dpa<PFN_vkUnmapMemory>(f, d, "vkUnmapMemory"),
        .AllocateCommandBuffers = dpa<PFN_vkAllocateCommandBuffers>(f, d,
            "vkAllocateCommandBuffers"),
        .FreeCommandBuffers = dpa<PFN_vkFreeCommandBuffers>(f, d, "vkFreeCommandBuffers"),
        .BeginCommandBuffer = dpa<PFN_vkBeginCommandBuffer>(f, d, "vkBeginCommandBuffer"),
        .EndCommandBuffer = dpa<PFN_vkEndCommandBuffer>(f, d, "vkEndCommandBuffer"),
        .CmdPipelineBarrier = dpa<PFN_vkCmdPipelineBarrier>(f, d, "vkCmdPipelineBarrier"),
        .CmdClearColorImage = dpa<PFN_vkCmdClearColorImage>(f, d, "vkCmdClearColorImage"),
        .CmdBindPipeline = dpa<PFN_vkCmdBindPipeline>(f, d, "vkCmdBindPipeline"),
        .CmdBindDescriptorSets = dpa<PFN_vkCmdBindDescriptorSets>(f, d, "vkCmdBindDescriptorSets"),
        .CmdDispatch = dpa<PFN_vkCmdDispatch>(f, d, "vkCmdDispatch"),
        .CmdCopyBufferToImage = dpa<PFN_vkCmdCopyBufferToImage>(f, d, "vkCmdCopyBufferToImage"),
        .QueueSubmit = dpa<PFN_vkQueueSubmit>(f, d, "vkQueueSubmit"),
        .AllocateDescriptorSets = dpa<PFN_vkAllocateDescriptorSets>(f, d,
            "vkAllocateDescriptorSets"),
        .FreeDescriptorSets = dpa<PFN_vkFreeDescriptorSets>(f, d, "vkFreeDescriptorSets"),
        .UpdateDescriptorSets = dpa<PFN_vkUpdateDescriptorSets>(f, d, "vkUpdateDescriptorSets"),
        .CreateFence = dpa<PFN_vkCreateFence>(f, d, "vkCreateFence"),
        .DestroyFence = dpa<PFN_vkDestroyFence>(f, d, "vkDestroyFence"),
        .ResetFences = dpa<PFN_vkResetFences>(f, d, "vkResetFences"),
        .WaitForFences = dpa<PFN_vkWaitForFences>(f, d, "vkWaitForFences"),
        .CreateImage = dpa<PFN_vkCreateImage>(f, d, "vkCreateImage"),
        .DestroyImage = dpa<PFN_vkDestroyImage>(f, d, "vkDestroyImage"),
        .GetImageMemoryRequirements = dpa<PFN_vkGetImageMemoryRequirements>(f, d,
            "vkGetImageMemoryRequirements"),
        .BindImageMemory = dpa<PFN_vkBindImageMemory>(f, d, "vkBindImageMemory"),
        .CreateImageView = dpa<PFN_vkCreateImageView>(f, d, "vkCreateImageView"),
        .DestroyImageView = dpa<PFN_vkDestroyImageView>(f, d, "vkDestroyImageView"),
        .CreateSampler = dpa<PFN_vkCreateSampler>(f, d, "vkCreateSampler"),
        .DestroySampler = dpa<PFN_vkDestroySampler>(f, d, "vkDestroySampler"),
        .CreateSemaphore = dpa<PFN_vkCreateSemaphore>(f, d, "vkCreateSemaphore"),
        .DestroySemaphore = dpa<PFN_vkDestroySemaphore>(f, d, "vkDestroySemaphore"),
        .CreateShaderModule = dpa<PFN_vkCreateShaderModule>(f, d, "vkCreateShaderModule"),
        .DestroyShaderModule = dpa<PFN_vkDestroyShaderModule>(f, d, "vkDestroyShaderModule"),
        .CreateDescriptorSetLayout = dpa<PFN_vkCreateDescriptorSetLayout>(f, d,
            "vkCreateDescriptorSetLayout"),
        .DestroyDescriptorSetLayout = dpa<PFN_vkDestroyDescriptorSetLayout>(f, d,
            "vkDestroyDescriptorSetLayout"),
        .CreatePipelineLayout = dpa<PFN_vkCreatePipelineLayout>(f, d, "vkCreatePipelineLayout"),
        .DestroyPipelineLayout = dpa<PFN_vkDestroyPipelineLayout>(f, d, "vkDestroyPipelineLayout"),
        .CreateComputePipelines = dpa<PFN_vkCreateComputePipelines>(f, d,
            "vkCreateComputePipelines"),
        .DestroyPipeline = dpa<PFN_vkDestroyPipeline>(f, d, "vkDestroyPipeline"),
        .SignalSemaphore = dpa<PFN_vkSignalSemaphore>(f, d, "vkSignalSemaphore"),
        .WaitSemaphores = dpa<PFN_vkWaitSemaphores>(f, d, "vkWaitSemaphores"),

        .GetMemoryFdKHR = dpa<PFN_vkGetMemoryFdKHR>(f, d, "vkGetMemoryFdKHR"),
        .ImportSemaphoreFdKHR = dpa<PFN_vkImportSemaphoreFdKHR>(f, d, "vkImportSemaphoreFdKHR"),
        .GetSemaphoreFdKHR = dpa<PFN_vkGetSemaphoreFdKHR>(f, d, "vkGetSemaphoreFdKHR"),
    };
}

Vulkan::Vulkan(const std::string& appName, version appVersion,
        const std::string& engineName, version engineVersion,
        PhysicalDeviceSelector selectPhysicalDevice) :
    instance(createInstance(
        appName, appVersion,
        engineName, engineVersion
    )),
    instance_funcs(initVulkanInstanceFuncs(*this->instance, get_mpa())),
    physdev(findPhysicalDevice(this->instance_funcs,
        *this->instance,
        selectPhysicalDevice
    )),
    computeFamilyIdx(findQFI(this->instance_funcs, this->physdev, VK_QUEUE_COMPUTE_BIT)),
    fp16(checkFP16(this->instance_funcs, this->physdev)),
    device(createLogicalDevice(this->instance_funcs,
        this->physdev,
        this->computeFamilyIdx,
        this->fp16
    )),
    device_funcs(initVulkanDeviceFuncs(
        this->instance_funcs,
        *this->device
    )),
    computeQueue(getQueue(this->device_funcs, *this->device, this->computeFamilyIdx)),
    cmdPool(createCommandPool(this->device_funcs,
        *this->device,
        this->computeFamilyIdx
    )),
    descPool(createDescriptorPool(this->device_funcs,
        *this->device
    )) {
}

std::optional<uint32_t> Vulkan::findMemoryTypeIndex(
        std::bitset<32> validTypes, bool hostVisibility) const {
    const VkMemoryPropertyFlags desiredProps = hostVisibility ?
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) :
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkPhysicalDeviceMemoryProperties props;
    this->instance_funcs.GetPhysicalDeviceMemoryProperties(this->physdev, &props);

    std::array<VkMemoryType, 32> memTypes = std::to_array(props.memoryTypes);
    for (uint32_t i = 0; i < props.memoryTypeCount; ++i)
        if (validTypes.test(i) && (memTypes.at(i).propertyFlags & desiredProps) == desiredProps)
            return i;

    return std::nullopt;
}
