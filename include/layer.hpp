#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace Layer {
    /// Call to the original vkCreateInstance function.
    VkResult ovkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance);
    /// Call to the original vkDestroyInstance function.
    void ovkDestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator);

    /// Call to the original vkCreateDevice function.
    VkResult ovkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice);
    /// Call to the original vkDestroyDevice function.
    void ovkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator);

    /// Call to the original vkSetDeviceLoaderData function.
    VkResult ovkSetDeviceLoaderData(
        VkDevice device,
        void* object);

    /// Call to the original vkGetInstanceProcAddr function.
    PFN_vkVoidFunction ovkGetInstanceProcAddr(
        VkInstance instance,
        const char* pName);
    /// Call to the original vkGetDeviceProcAddr function.
    PFN_vkVoidFunction ovkGetDeviceProcAddr(
        VkDevice device,
        const char* pName);

    /// Call to the original vkGetPhysicalDeviceQueueFamilyProperties function.
    void ovkGetPhysicalDeviceQueueFamilyProperties(
        VkPhysicalDevice physicalDevice,
        uint32_t* pQueueFamilyPropertyCount,
        VkQueueFamilyProperties* pQueueFamilyProperties);
    /// Call to the original vkGetPhysicalDeviceMemoryProperties function.
    void ovkGetPhysicalDeviceMemoryProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceMemoryProperties* pMemoryProperties);
    /// Call to the original vkGetPhysicalDeviceProperties function.
    void ovkGetPhysicalDeviceProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties* pProperties);
    /// Call to the original vkGetPhysicalDeviceSurfaceCapabilitiesKHR function.
    VkResult ovkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        VkSurfaceCapabilitiesKHR* pSurfaceCapabilities);

    /// Call to the original vkCreateSwapchainKHR function.
    VkResult ovkCreateSwapchainKHR(
        VkDevice device,
        const VkSwapchainCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSwapchainKHR* pSwapchain);
    /// Call to the original vkQueuePresentKHR function.
    VkResult ovkQueuePresentKHR(
        VkQueue queue,
        const VkPresentInfoKHR* pPresentInfo);
    /// Call to the original vkDestroySwapchainKHR function.
    void ovkDestroySwapchainKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        const VkAllocationCallbacks* pAllocator);

    /// Call to the original vkGetSwapchainImagesKHR function.
    VkResult ovkGetSwapchainImagesKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        uint32_t* pSwapchainImageCount,
        VkImage* pSwapchainImages);

    /// Call to the original vkAllocateCommandBuffers function.
    VkResult ovkAllocateCommandBuffers(
        VkDevice device,
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers);
    /// Call to the original vkFreeCommandBuffers function.
    void ovkFreeCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers);

    /// Call to the original vkBeginCommandBuffer function.
    VkResult ovkBeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo);
    /// Call to the original vkEndCommandBuffer function.
    VkResult ovkEndCommandBuffer(
        VkCommandBuffer commandBuffer);

    /// Call to the original vkCreateCommandPool function.
    VkResult ovkCreateCommandPool(
        VkDevice device,
        const VkCommandPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkCommandPool* pCommandPool);
    /// Call to the original vkDestroyCommandPool function.
    void ovkDestroyCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        const VkAllocationCallbacks* pAllocator);

    /// Call to the original vkCreateImage function.
    VkResult ovkCreateImage(
        VkDevice device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage);
    /// Call to the original vkDestroyImage function.
    void ovkDestroyImage(
        VkDevice device,
        VkImage image,
        const VkAllocationCallbacks* pAllocator);

    /// Call to the original vkGetImageMemoryRequirements function.
    void ovkGetImageMemoryRequirements(
        VkDevice device,
        VkImage image,
        VkMemoryRequirements* pMemoryRequirements);
    /// Call to the original vkBindImageMemory function.
    VkResult ovkBindImageMemory(
        VkDevice device,
        VkImage image,
        VkDeviceMemory memory,
        VkDeviceSize memoryOffset);
    /// Call to the original vkAllocateMemory function.
    VkResult ovkAllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory);
    /// Call to the original vkFreeMemory function.
    void ovkFreeMemory(
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator);

    /// Call to the original vkCreateSemaphore function.
    VkResult ovkCreateSemaphore(
        VkDevice device,
        const VkSemaphoreCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSemaphore* pSemaphore);
    /// Call to the original vkDestroySemaphore function.
    void ovkDestroySemaphore(
        VkDevice device,
        VkSemaphore semaphore,
        const VkAllocationCallbacks* pAllocator);

    /// Call to the original vkGetMemoryFdKHR function.
    VkResult ovkGetMemoryFdKHR(
        VkDevice device,
        const VkMemoryGetFdInfoKHR* pGetFdInfo,
        int* pFd);
    /// Call to the original vkGetSemaphoreFdKHR function.
    VkResult ovkGetSemaphoreFdKHR(
        VkDevice device,
        const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
        int* pFd);

    /// Call to the original vkGetDeviceQueue function.
    void ovkGetDeviceQueue(
        VkDevice device,
        uint32_t queueFamilyIndex,
        uint32_t queueIndex,
        VkQueue* pQueue);
    /// Call to the original vkQueueSubmit function.
    VkResult ovkQueueSubmit(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo* pSubmits,
        VkFence fence);

    /// Call to the original vkCmdPipelineBarrier function.
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
        const VkImageMemoryBarrier* pImageMemoryBarriers);
    /// Call to the original vkCmdBlitImage function.
    void ovkCmdBlitImage(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkImageBlit* pRegions,
        VkFilter filter);

    /// Call to the original vkAcquireNextImageKHR function.
    VkResult ovkAcquireNextImageKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        uint64_t timeout,
        VkSemaphore semaphore,
        VkFence fence,
        uint32_t* pImageIndex);
}

/// Symbol definition for Vulkan instance layer.
__attribute__((visibility("default")))
extern "C" PFN_vkVoidFunction layer_vkGetInstanceProcAddr(VkInstance instance, const char* pName);
/// Symbol definition for Vulkan device layer.
__attribute__((visibility("default")))
extern "C" PFN_vkVoidFunction layer_vkGetDeviceProcAddr(VkDevice device, const char* pName);
