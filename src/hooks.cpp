#include "hooks.hpp"
#include "common/exception.hpp"
#include "utils/utils.hpp"
#include "context.hpp"
#include "layer.hpp"

#include <iostream>
#include <vulkan/vulkan_core.h>

#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <exception>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Hooks;

namespace {

    ///
    /// Add extensions to the instance create info.
    ///
    VkResult myvkCreateInstance(
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {
        auto extensions = Utils::addExtensions(
            pCreateInfo->ppEnabledExtensionNames,
            pCreateInfo->enabledExtensionCount,
            {
                "VK_KHR_get_physical_device_properties2",
                "VK_KHR_external_memory_capabilities",
                "VK_KHR_external_semaphore_capabilities"
            }
        );
        VkInstanceCreateInfo createInfo = *pCreateInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        auto res = Layer::ovkCreateInstance(&createInfo, pAllocator, pInstance);
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
            throw std::runtime_error(
                "Required Vulkan instance extensions are not present."
                "Your GPU driver is not supported.");
        return res;
    }

    /// Map of devices to related information.
    std::unordered_map<VkDevice, DeviceInfo> deviceToInfo;

    ///
    /// Add extensions to the device create info.
    /// (function pointers are not initialized yet)
    ///
    VkResult myvkCreateDevicePre(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        // add extensions
        auto extensions = Utils::addExtensions(
            pCreateInfo->ppEnabledExtensionNames,
            pCreateInfo->enabledExtensionCount,
            {
                "VK_KHR_external_memory",
                "VK_KHR_external_memory_fd",
                "VK_KHR_external_semaphore",
                "VK_KHR_external_semaphore_fd"
            }
        );
        VkDeviceCreateInfo createInfo = *pCreateInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        auto res = Layer::ovkCreateDevice(physicalDevice, &createInfo, pAllocator, pDevice);
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
            throw std::runtime_error(
                "Required Vulkan device extensions are not present."
                "Your GPU driver is not supported.");
        return res;
    }

    ///
    /// Add related device information after the device is created.
    ///
    VkResult myvkCreateDevicePost(
            VkPhysicalDevice physicalDevice,
            VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks*,
            VkDevice* pDevice) {
        deviceToInfo.emplace(*pDevice, DeviceInfo {
            .device = *pDevice,
            .physicalDevice = physicalDevice,
            .queue = Utils::findQueue(*pDevice, physicalDevice, pCreateInfo, VK_QUEUE_GRAPHICS_BIT)
        });
        return VK_SUCCESS;
    }

    /// Erase the device information when the device is destroyed.
    void myvkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) noexcept {
        deviceToInfo.erase(device);
        Layer::ovkDestroyDevice(device, pAllocator);
    }

    std::unordered_map<VkSwapchainKHR, LsContext> swapchains;
    std::unordered_map<VkSwapchainKHR, VkDevice> swapchainToDeviceTable;

    ///
    /// Adjust swapchain creation parameters and create a swapchain context.
    ///
    VkResult myvkCreateSwapchainKHR(
            VkDevice device,
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSwapchainKHR* pSwapchain) noexcept {
        // find device
        auto it = deviceToInfo.find(device);
        if (it == deviceToInfo.end()) {
            Utils::logLimitN("swapMap", 5, "Device not found in map");
            return Layer::ovkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
        }
        Utils::resetLimitN("swapMap");
        auto& deviceInfo = it->second;

        // increase amount of images in swapchain
        VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;
        const auto maxImages = Utils::getMaxImageCount(
            deviceInfo.physicalDevice, pCreateInfo->surface);
        createInfo.minImageCount = createInfo.minImageCount + 1
            + static_cast<uint32_t>(deviceInfo.queue.first);
        if (createInfo.minImageCount > maxImages) {
            createInfo.minImageCount = maxImages;
            Utils::logLimitN("swapCount", 10,
                "Requested image count (" +
                    std::to_string(pCreateInfo->minImageCount) + ") "
                "exceeds maximum allowed (" +
                    std::to_string(maxImages) + "). "
                "Continuing with maximum allowed image count. "
                "This might lead to performance degradation.");
        } else {
            Utils::resetLimitN("swapCount");
        }

        // allow copy operations on swapchain images
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        // enforce vsync
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

        // retire potential old swapchain
        if (pCreateInfo->oldSwapchain) {
            swapchains.erase(pCreateInfo->oldSwapchain);
            swapchainToDeviceTable.erase(pCreateInfo->oldSwapchain);
        }

        // create swapchain
        auto res = Layer::ovkCreateSwapchainKHR(device, &createInfo, pAllocator, pSwapchain);
        if (res != VK_SUCCESS)
            return res; // can't be caused by lsfg-vk (yet)

        try {
            // get all swapchain images
            uint32_t imageCount{};
            res = Layer::ovkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, nullptr);
            if (res != VK_SUCCESS || imageCount == 0)
                throw LSFG::vulkan_error(res, "Failed to get swapchain image count");

            std::vector<VkImage> swapchainImages(imageCount);
            res = Layer::ovkGetSwapchainImagesKHR(device, *pSwapchain,
                &imageCount, swapchainImages.data());
            if (res != VK_SUCCESS)
                throw LSFG::vulkan_error(res, "Failed to get swapchain images");

            // create swapchain context
            swapchainToDeviceTable.emplace(*pSwapchain, device);
            swapchains.emplace(*pSwapchain, LsContext(
                deviceInfo, *pSwapchain, pCreateInfo->imageExtent,
                swapchainImages
            ));

            std::cerr << "lsfg-vk: Swapchain context " <<
                    (createInfo.oldSwapchain ? "recreated" : "created")
                << " (using " << imageCount << " images).\n";

            Utils::resetLimitN("swapCtxCreate");
        } catch (const std::exception& e) {
            Utils::logLimitN("swapCtxCreate", 5,
                "An error occurred while creating the swapchain wrapper:\n"
                "- " + std::string(e.what()));
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return VK_SUCCESS;
    }

    ///
    /// Update presentation parameters and present the next frame(s).
    ///
    VkResult myvkQueuePresentKHR(
            VkQueue queue,
            const VkPresentInfoKHR* pPresentInfo) noexcept {
        // find swapchain device
        auto it = swapchainToDeviceTable.find(*pPresentInfo->pSwapchains);
        if (it == swapchainToDeviceTable.end()) {
            Utils::logLimitN("swapMap", 5,
                "Swapchain not found in map");
            return Layer::ovkQueuePresentKHR(queue, pPresentInfo);
        }

        // find device info
        auto it2 = deviceToInfo.find(it->second);
        if (it2 == deviceToInfo.end()) {
            Utils::logLimitN("swapMap", 5,
                "Device not found in map");
            return Layer::ovkQueuePresentKHR(queue, pPresentInfo);
        }
        auto& deviceInfo = it2->second;

        // find swapchain context
        auto it3 = swapchains.find(*pPresentInfo->pSwapchains);
        if (it3 == swapchains.end()) {
            Utils::logLimitN("swapMap", 5,
                "Swapchain context not found in map");
            return Layer::ovkQueuePresentKHR(queue, pPresentInfo);
        }
        auto& swapchain = it3->second;

        // enforce vsync | NOLINTBEGIN
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        const VkSwapchainPresentModeInfoEXT* presentModeInfo =
            reinterpret_cast<const VkSwapchainPresentModeInfoEXT*>(pPresentInfo->pNext);
        while (presentModeInfo) {
            if (presentModeInfo->sType == VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT) {
                for (size_t i = 0; i < presentModeInfo->swapchainCount; i++)
                    const_cast<VkPresentModeKHR*>(presentModeInfo->pPresentModes)[i] =
                        VK_PRESENT_MODE_FIFO_KHR;
            }
            presentModeInfo =
                reinterpret_cast<const VkSwapchainPresentModeInfoEXT*>(presentModeInfo->pNext);
        }
        #pragma clang diagnostic pop

        // NOLINTEND | present the next frame
        VkResult res{}; // might return VK_SUBOPTIMAL_KHR
        try {
            std::vector<VkSemaphore> semaphores(pPresentInfo->waitSemaphoreCount);
            std::copy_n(pPresentInfo->pWaitSemaphores, semaphores.size(), semaphores.data());

            res = swapchain.present(deviceInfo, pPresentInfo->pNext,
                queue, semaphores, *pPresentInfo->pImageIndices);

            Utils::resetLimitN("swapPresent");
        } catch (const std::exception& e) {
            Utils::logLimitN("swapPresent", 5,
                "An error occurred while presenting the swapchain:\n"
                "- " + std::string(e.what()));
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return res;
    }

    /// Erase the swapchain context and mapping when the swapchain is destroyed.
    void myvkDestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* pAllocator) noexcept {
        swapchains.erase(swapchain);
        swapchainToDeviceTable.erase(swapchain);
        Layer::ovkDestroySwapchainKHR(device, swapchain, pAllocator);
    }
}

std::unordered_map<std::string, PFN_vkVoidFunction> Hooks::hooks = {
    // instance hooks
    {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(myvkCreateInstance)},

    // device hooks
    {"vkCreateDevicePre", reinterpret_cast<PFN_vkVoidFunction>(myvkCreateDevicePre)},
    {"vkCreateDevicePost", reinterpret_cast<PFN_vkVoidFunction>(myvkCreateDevicePost)},
    {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(myvkDestroyDevice)},

    // swapchain hooks
    {"vkCreateSwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(myvkCreateSwapchainKHR)},
    {"vkQueuePresentKHR", reinterpret_cast<PFN_vkVoidFunction>(myvkQueuePresentKHR)},
    {"vkDestroySwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(myvkDestroySwapchainKHR)}
};
