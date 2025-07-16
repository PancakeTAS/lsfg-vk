#include "layer.hpp"
#include "common/exception.hpp"
#include "config/config.hpp"
#include "hooks.hpp"

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

#include <unordered_map>
#include <exception>
#include <iostream>
#include <cstdint>
#include <string>

namespace {
    PFN_vkCreateInstance  next_vkCreateInstance{};
    PFN_vkDestroyInstance next_vkDestroyInstance{};

    PFN_vkCreateDevice  next_vkCreateDevice{};
    PFN_vkDestroyDevice next_vkDestroyDevice{};

    PFN_vkSetDeviceLoaderData next_vSetDeviceLoaderData{};

    PFN_vkGetInstanceProcAddr next_vkGetInstanceProcAddr{};
    PFN_vkGetDeviceProcAddr   next_vkGetDeviceProcAddr{};

    PFN_vkGetPhysicalDeviceQueueFamilyProperties next_vkGetPhysicalDeviceQueueFamilyProperties{};
    PFN_vkGetPhysicalDeviceMemoryProperties next_vkGetPhysicalDeviceMemoryProperties{};
    PFN_vkGetPhysicalDeviceProperties next_vkGetPhysicalDeviceProperties{};
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR next_vkGetPhysicalDeviceSurfaceCapabilitiesKHR{};

    PFN_vkCreateSwapchainKHR  next_vkCreateSwapchainKHR{};
    PFN_vkQueuePresentKHR     next_vkQueuePresentKHR{};
    PFN_vkDestroySwapchainKHR next_vkDestroySwapchainKHR{};
    PFN_vkGetSwapchainImagesKHR  next_vkGetSwapchainImagesKHR{};
    PFN_vkAllocateCommandBuffers next_vkAllocateCommandBuffers{};
    PFN_vkFreeCommandBuffers     next_vkFreeCommandBuffers{};
    PFN_vkBeginCommandBuffer next_vkBeginCommandBuffer{};
    PFN_vkEndCommandBuffer   next_vkEndCommandBuffer{};
    PFN_vkCreateCommandPool  next_vkCreateCommandPool{};
    PFN_vkDestroyCommandPool next_vkDestroyCommandPool{};
    PFN_vkCreateImage  next_vkCreateImage{};
    PFN_vkDestroyImage next_vkDestroyImage{};
    PFN_vkGetImageMemoryRequirements next_vkGetImageMemoryRequirements{};
    PFN_vkBindImageMemory next_vkBindImageMemory{};
    PFN_vkAllocateMemory  next_vkAllocateMemory{};
    PFN_vkFreeMemory next_vkFreeMemory{};
    PFN_vkCreateSemaphore  next_vkCreateSemaphore{};
    PFN_vkDestroySemaphore next_vkDestroySemaphore{};
    PFN_vkGetMemoryFdKHR next_vkGetMemoryFdKHR{};
    PFN_vkGetSemaphoreFdKHR next_vkGetSemaphoreFdKHR{};
    PFN_vkGetDeviceQueue next_vkGetDeviceQueue{};
    PFN_vkQueueSubmit next_vkQueueSubmit{};
    PFN_vkCmdPipelineBarrier next_vkCmdPipelineBarrier{};
    PFN_vkCmdBlitImage next_vkCmdBlitImage{};
    PFN_vkAcquireNextImageKHR next_vkAcquireNextImageKHR{};

    template<typename T>
    bool initInstanceFunc(VkInstance instance, const char* name, T* func) {
        *func = reinterpret_cast<T>(next_vkGetInstanceProcAddr(instance, name));
        if (!*func) {
            std::cerr << "(no function pointer for " << name << ")\n";
            return false;
        }
        return true;
    }

    template<typename T>
    bool initDeviceFunc(VkDevice device, const char* name, T* func) {
        *func = reinterpret_cast<T>(next_vkGetDeviceProcAddr(device, name));
        if (!*func) {
            std::cerr << "(no function pointer for " << name << ")\n";
            return false;
        }
        return true;
    }
}

namespace {
    VkResult layer_vkCreateInstance(
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {
        try {
            // prepare layer | NOLINTBEGIN
            auto* layerDesc = const_cast<VkLayerInstanceCreateInfo*>(
                reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext));
            while (layerDesc && (layerDesc->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO
                    || layerDesc->function != VK_LAYER_LINK_INFO)) {
                layerDesc = const_cast<VkLayerInstanceCreateInfo*>(
                    reinterpret_cast<const VkLayerInstanceCreateInfo*>(layerDesc->pNext));
            }
            if (!layerDesc)
                throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED,
                    "No layer creation info found in pNext chain");

            next_vkGetInstanceProcAddr = layerDesc->u.pLayerInfo->pfnNextGetInstanceProcAddr;
            layerDesc->u.pLayerInfo = layerDesc->u.pLayerInfo->pNext;

            bool success = initInstanceFunc(nullptr, "vkCreateInstance", &next_vkCreateInstance);
            if (!success)
                throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED,
                    "Failed to get instance function pointer for vkCreateInstance");
            success &= initInstanceFunc(*pInstance,
                "vkCreateDevice", &next_vkCreateDevice);
            if (!success)
                throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED,
                    "Failed to get instance function pointer for vkCreateDevice");

            // NOLINTEND | skip initialization if the layer is disabled
            if (!Config::activeConf.enable)
                return next_vkCreateInstance(pCreateInfo, pAllocator, pInstance);

            // create instance
            try {
                auto* createInstanceHook = reinterpret_cast<PFN_vkCreateInstance>(
                    Hooks::hooks["vkCreateInstance"]);
                auto res = createInstanceHook(pCreateInfo, pAllocator, pInstance);
                if (res != VK_SUCCESS)
                    throw LSFG::vulkan_error(res, "Unknown error");
            } catch (const std::exception& e) {
                throw LSFG::rethrowable_error("Failed to create Vulkan instance", e);
            }

            // get relevant function pointers from the next layer
            success = true;
            success &= initInstanceFunc(*pInstance,
                "vkDestroyInstance", &next_vkDestroyInstance);
            success &= initInstanceFunc(*pInstance,
                "vkGetPhysicalDeviceQueueFamilyProperties", &next_vkGetPhysicalDeviceQueueFamilyProperties);
            success &= initInstanceFunc(*pInstance,
                "vkGetPhysicalDeviceMemoryProperties", &next_vkGetPhysicalDeviceMemoryProperties);
            success &= initInstanceFunc(*pInstance,
                "vkGetPhysicalDeviceProperties", &next_vkGetPhysicalDeviceProperties);
            success &= initInstanceFunc(*pInstance,
                "vkGetPhysicalDeviceSurfaceCapabilitiesKHR", &next_vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
            if (!success)
                throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED,
                    "Failed to get instance function pointers");

            std::cerr << "lsfg-vk: Vulkan instance layer initialized successfully.\n";
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occurred while initializing the Vulkan instance layer:\n";
            std::cerr << "- " << e.what() << '\n';
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return VK_SUCCESS;
    }

    VkResult layer_vkCreateDevice( // NOLINTBEGIN
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        try {
            // prepare layer | NOLINTBEGIN
            auto* layerDesc = const_cast<VkLayerDeviceCreateInfo*>(
                reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext));
            while (layerDesc && (layerDesc->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
                    || layerDesc->function != VK_LAYER_LINK_INFO)) {
                layerDesc = const_cast<VkLayerDeviceCreateInfo*>(
                    reinterpret_cast<const VkLayerDeviceCreateInfo*>(layerDesc->pNext));
            }
            if (!layerDesc)
                throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED,
                    "No layer creation info found in pNext chain");

            next_vkGetDeviceProcAddr = layerDesc->u.pLayerInfo->pfnNextGetDeviceProcAddr;
            layerDesc->u.pLayerInfo = layerDesc->u.pLayerInfo->pNext;

            auto* layerDesc2 = const_cast<VkLayerDeviceCreateInfo*>(
                reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext));
            while (layerDesc2 && (layerDesc2->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
                    || layerDesc2->function != VK_LOADER_DATA_CALLBACK)) {
                        layerDesc2 = const_cast<VkLayerDeviceCreateInfo*>(
                            reinterpret_cast<const VkLayerDeviceCreateInfo*>(layerDesc2->pNext));
            }
            if (!layerDesc2)
                throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED,
                    "No layer device loader data found in pNext chain");

            next_vSetDeviceLoaderData = layerDesc2->u.pfnSetDeviceLoaderData;

            // NOLINTEND | skip initialization if the layer is disabled
            if (!Config::activeConf.enable)
                return next_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

            // create device
            try {
                auto* createDeviceHook = reinterpret_cast<PFN_vkCreateDevice>(
                    Hooks::hooks["vkCreateDevicePre"]);
                auto res = createDeviceHook(physicalDevice, pCreateInfo, pAllocator, pDevice);
                if (res != VK_SUCCESS)
                    throw LSFG::vulkan_error(res, "Unknown error");
            } catch (const std::exception& e) {
                throw LSFG::rethrowable_error("Failed to create Vulkan device", e);
            }

            // get relevant function pointers from the next layer
            bool success = true;
            success &= initDeviceFunc(*pDevice, "vkDestroyDevice", &next_vkDestroyDevice);
            success &= initDeviceFunc(*pDevice, "vkCreateSwapchainKHR", &next_vkCreateSwapchainKHR);
            success &= initDeviceFunc(*pDevice, "vkQueuePresentKHR", &next_vkQueuePresentKHR);
            success &= initDeviceFunc(*pDevice, "vkDestroySwapchainKHR", &next_vkDestroySwapchainKHR);
            success &= initDeviceFunc(*pDevice, "vkGetSwapchainImagesKHR", &next_vkGetSwapchainImagesKHR);
            success &= initDeviceFunc(*pDevice, "vkAllocateCommandBuffers", &next_vkAllocateCommandBuffers);
            success &= initDeviceFunc(*pDevice, "vkFreeCommandBuffers", &next_vkFreeCommandBuffers);
            success &= initDeviceFunc(*pDevice, "vkBeginCommandBuffer", &next_vkBeginCommandBuffer);
            success &= initDeviceFunc(*pDevice, "vkEndCommandBuffer", &next_vkEndCommandBuffer);
            success &= initDeviceFunc(*pDevice, "vkCreateCommandPool", &next_vkCreateCommandPool);
            success &= initDeviceFunc(*pDevice, "vkDestroyCommandPool", &next_vkDestroyCommandPool);
            success &= initDeviceFunc(*pDevice, "vkCreateImage", &next_vkCreateImage);
            success &= initDeviceFunc(*pDevice, "vkDestroyImage", &next_vkDestroyImage);
            success &= initDeviceFunc(*pDevice, "vkGetImageMemoryRequirements", &next_vkGetImageMemoryRequirements);
            success &= initDeviceFunc(*pDevice, "vkBindImageMemory", &next_vkBindImageMemory);
            success &= initDeviceFunc(*pDevice, "vkGetMemoryFdKHR", &next_vkGetMemoryFdKHR);
            success &= initDeviceFunc(*pDevice, "vkAllocateMemory", &next_vkAllocateMemory);
            success &= initDeviceFunc(*pDevice, "vkFreeMemory", &next_vkFreeMemory);
            success &= initDeviceFunc(*pDevice, "vkCreateSemaphore", &next_vkCreateSemaphore);
            success &= initDeviceFunc(*pDevice, "vkDestroySemaphore", &next_vkDestroySemaphore);
            success &= initDeviceFunc(*pDevice, "vkGetSemaphoreFdKHR", &next_vkGetSemaphoreFdKHR);
            success &= initDeviceFunc(*pDevice, "vkGetDeviceQueue", &next_vkGetDeviceQueue);
            success &= initDeviceFunc(*pDevice, "vkQueueSubmit", &next_vkQueueSubmit);
            success &= initDeviceFunc(*pDevice, "vkCmdPipelineBarrier", &next_vkCmdPipelineBarrier);
            success &= initDeviceFunc(*pDevice, "vkCmdBlitImage", &next_vkCmdBlitImage);
            success &= initDeviceFunc(*pDevice, "vkAcquireNextImageKHR", &next_vkAcquireNextImageKHR);
            if (!success)
                throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED,
                    "Failed to get device function pointers");

            auto postCreateDeviceHook = reinterpret_cast<PFN_vkCreateDevice>(
                Hooks::hooks["vkCreateDevicePost"]);
            auto res = postCreateDeviceHook(physicalDevice, pCreateInfo, pAllocator, pDevice);
            if (res != VK_SUCCESS)
                throw LSFG::vulkan_error(res, "Unknown error");

            std::cerr << "lsfg-vk: Vulkan device layer initialized successfully.\n";
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occurred while initializing the Vulkan device layer:\n";
            std::cerr << "- " << e.what() << '\n';
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return VK_SUCCESS;
    } // NOLINTEND
}

const std::unordered_map<std::string, PFN_vkVoidFunction> layerFunctions = {
    { "vkCreateInstance",
        reinterpret_cast<PFN_vkVoidFunction>(&layer_vkCreateInstance) },
    { "vkCreateDevice",
        reinterpret_cast<PFN_vkVoidFunction>(&layer_vkCreateDevice) },
    { "vkGetInstanceProcAddr",
        reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetInstanceProcAddr) },
    { "vkGetDeviceProcAddr",
        reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetDeviceProcAddr) },
};

PFN_vkVoidFunction layer_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    const std::string name(pName);
    auto it = layerFunctions.find(name);
    if (it != layerFunctions.end())
        return it->second;

    it = Hooks::hooks.find(name);
    if (it != Hooks::hooks.end() && Config::activeConf.enable)
        return it->second;

    return next_vkGetInstanceProcAddr(instance, pName);
}

PFN_vkVoidFunction layer_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    const std::string name(pName);
    auto it = layerFunctions.find(name);
    if (it != layerFunctions.end())
        return it->second;

    it = Hooks::hooks.find(name);
    if (it != Hooks::hooks.end() && Config::activeConf.enable)
        return it->second;

    return next_vkGetDeviceProcAddr(device, pName);
}

// original functions
namespace Layer {
    VkResult ovkCreateInstance(
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {
        return next_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
    }
    void ovkDestroyInstance(
            VkInstance instance,
            const VkAllocationCallbacks* pAllocator) {
        next_vkDestroyInstance(instance, pAllocator);
    }

    VkResult ovkCreateDevice(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        return next_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    }
    void ovkDestroyDevice(
            VkDevice device,
            const VkAllocationCallbacks* pAllocator) {
        next_vkDestroyDevice(device, pAllocator);
    }

    VkResult ovkSetDeviceLoaderData(VkDevice device, void* object) {
        return next_vSetDeviceLoaderData(device, object);
    }

    PFN_vkVoidFunction ovkGetInstanceProcAddr(
            VkInstance instance,
            const char* pName) {
        return next_vkGetInstanceProcAddr(instance, pName);
    }
    PFN_vkVoidFunction ovkGetDeviceProcAddr(
            VkDevice device,
            const char* pName) {
        return next_vkGetDeviceProcAddr(device, pName);
    }

    void ovkGetPhysicalDeviceQueueFamilyProperties(
            VkPhysicalDevice physicalDevice,
            uint32_t* pQueueFamilyPropertyCount,
            VkQueueFamilyProperties* pQueueFamilyProperties) {
        next_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    }
    void ovkGetPhysicalDeviceMemoryProperties(
            VkPhysicalDevice physicalDevice,
            VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
        next_vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
    }
    void ovkGetPhysicalDeviceProperties(
            VkPhysicalDevice physicalDevice,
            VkPhysicalDeviceProperties* pProperties) {
        next_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
    }
    VkResult ovkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            VkPhysicalDevice physicalDevice,
            VkSurfaceKHR surface,
            VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) {
        return next_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
    }

    VkResult ovkCreateSwapchainKHR(
            VkDevice device,
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSwapchainKHR* pSwapchain) {
        return next_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    }
    VkResult ovkQueuePresentKHR(
            VkQueue queue,
            const VkPresentInfoKHR* pPresentInfo) {
        return next_vkQueuePresentKHR(queue, pPresentInfo);
    }
    void ovkDestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* pAllocator) {
        next_vkDestroySwapchainKHR(device, swapchain, pAllocator);
    }

    VkResult ovkGetSwapchainImagesKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            uint32_t* pSwapchainImageCount,
            VkImage* pSwapchainImages) {
        return next_vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    }

    VkResult ovkAllocateCommandBuffers(
            VkDevice device,
            const VkCommandBufferAllocateInfo* pAllocateInfo,
            VkCommandBuffer* pCommandBuffers) {
        return next_vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    }
    void ovkFreeCommandBuffers(
            VkDevice device,
            VkCommandPool commandPool,
            uint32_t commandBufferCount,
            const VkCommandBuffer* pCommandBuffers) {
        next_vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
    }

    VkResult ovkBeginCommandBuffer(
            VkCommandBuffer commandBuffer,
            const VkCommandBufferBeginInfo* pBeginInfo) {
        return next_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
    }
    VkResult ovkEndCommandBuffer(
            VkCommandBuffer commandBuffer) {
        return next_vkEndCommandBuffer(commandBuffer);
    }

    VkResult ovkCreateCommandPool(
            VkDevice device,
            const VkCommandPoolCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkCommandPool* pCommandPool) {
        return  next_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    }
    void ovkDestroyCommandPool(
            VkDevice device,
            VkCommandPool commandPool,
            const VkAllocationCallbacks* pAllocator) {
        next_vkDestroyCommandPool(device, commandPool, pAllocator);
    }

    VkResult ovkCreateImage(
            VkDevice device,
            const VkImageCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkImage* pImage) {
        return next_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
    }
    void ovkDestroyImage(
            VkDevice device,
            VkImage image,
            const VkAllocationCallbacks* pAllocator) {
        next_vkDestroyImage(device, image, pAllocator);
    }

    void ovkGetImageMemoryRequirements(
            VkDevice device,
            VkImage image,
            VkMemoryRequirements* pMemoryRequirements) {
        next_vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
    }
    VkResult ovkBindImageMemory(
            VkDevice device,
            VkImage image,
            VkDeviceMemory memory,
            VkDeviceSize memoryOffset) {
        return next_vkBindImageMemory(device, image, memory, memoryOffset);
    }

    VkResult ovkAllocateMemory(
            VkDevice device,
            const VkMemoryAllocateInfo* pAllocateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDeviceMemory* pMemory) {
        return next_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
    }
    void ovkFreeMemory(
            VkDevice device,
            VkDeviceMemory memory,
            const VkAllocationCallbacks* pAllocator) {
        next_vkFreeMemory(device, memory, pAllocator);
    }

    VkResult ovkCreateSemaphore(
            VkDevice device,
            const VkSemaphoreCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSemaphore* pSemaphore) {
        return next_vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    }
    void ovkDestroySemaphore(
            VkDevice device,
            VkSemaphore semaphore,
            const VkAllocationCallbacks* pAllocator) {
        next_vkDestroySemaphore(device, semaphore, pAllocator);
    }

    VkResult ovkGetMemoryFdKHR(
            VkDevice device,
            const VkMemoryGetFdInfoKHR* pGetFdInfo,
            int* pFd) {
        return next_vkGetMemoryFdKHR(device, pGetFdInfo, pFd);
    }
    VkResult ovkGetSemaphoreFdKHR(
            VkDevice device,
            const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
            int* pFd) {
        return next_vkGetSemaphoreFdKHR(device, pGetFdInfo, pFd);
    }

    void ovkGetDeviceQueue(
            VkDevice device,
            uint32_t queueFamilyIndex,
            uint32_t queueIndex,
            VkQueue* pQueue) {
        next_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    }
    VkResult ovkQueueSubmit(
            VkQueue queue,
            uint32_t submitCount,
            const VkSubmitInfo* pSubmits,
            VkFence fence) {
        return next_vkQueueSubmit(queue, submitCount, pSubmits, fence);
    }

    void ovkCmdPipelineBarrier(
            VkCommandBuffer commandBuffer,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkDependencyFlags dependencyFlags,
            uint32_t memoryBarrierCount,
            const VkMemoryBarrier* pMemoryBarriers,
            uint32_t bufferMemoryBarrierCount,
            const VkBufferMemoryBarrier* pBufferMemoryBarriers,
            uint32_t imageMemoryBarrierCount,
            const VkImageMemoryBarrier* pImageMemoryBarriers) {
        next_vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
            memoryBarrierCount, pMemoryBarriers,
            bufferMemoryBarrierCount, pBufferMemoryBarriers,
            imageMemoryBarrierCount, pImageMemoryBarriers);
    }
    void ovkCmdBlitImage(
            VkCommandBuffer commandBuffer,
            VkImage srcImage,
            VkImageLayout srcImageLayout,
            VkImage dstImage,
            VkImageLayout dstImageLayout,
            uint32_t regionCount,
            const VkImageBlit* pRegions,
            VkFilter filter) {
        next_vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
    }

    VkResult ovkAcquireNextImageKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            uint64_t timeout,
            VkSemaphore semaphore,
            VkFence fence,
            uint32_t* pImageIndex) {
        return next_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    }
}
