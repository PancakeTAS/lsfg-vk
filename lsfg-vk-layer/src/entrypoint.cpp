/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "hooks/device.hpp"
#include "hooks/instance.hpp"
#include "hooks/layer.hpp"
#include "hooks/swapchain.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk::layer;

namespace {
    // global layer info initialized at layer negotiation
    struct LayerInfo {
        std::unordered_map<std::string, PFN_vkVoidFunction> map; // layer override map

        PFN_vkGetInstanceProcAddr GetInstanceProcAddr; // next layer functions
        PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
        PFN_vkQueueSubmit QueueSubmit;

        MyVkLayer layer; // managed instances
        std::unordered_map<VkInstance, std::unique_ptr<MyVkInstance>> instances;
        std::unordered_map<VkDevice, std::unique_ptr<MyVkDevice>> devices;
        std::unordered_map<VkSwapchainKHR, std::unique_ptr<MyVkSwapchain>> swapchains;
    }* layer_info; // NOLINT (global variable)

    // create instance
    VkResult myvkCreateInstance(
            const VkInstanceCreateInfo* info,
            const VkAllocationCallbacks* alloc,
            VkInstance* instance) {
        auto& myvk_layer = layer_info->layer;

        // apply layer chaining
        auto* layerInfo = reinterpret_cast<VkLayerInstanceCreateInfo*>(const_cast<void*>(info->pNext));
        while (layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO
                || layerInfo->function != VK_LAYER_LINK_INFO)) {
            layerInfo = reinterpret_cast<VkLayerInstanceCreateInfo*>(const_cast<void*>(layerInfo->pNext));
        }
        if (!layerInfo) {
            std::cerr << "lsfg-vk: no layer info found in pNext chain, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto* linkInfo = layerInfo->u.pLayerInfo;
        if (!linkInfo) {
            std::cerr << "lsfg-vk: link info is null, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        layer_info->GetInstanceProcAddr = linkInfo->pfnNextGetInstanceProcAddr;
        if (!layer_info->GetInstanceProcAddr) {
            std::cerr << "lsfg-vk: next layer's vkGetInstanceProcAddr is null, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        layerInfo->u.pLayerInfo = linkInfo->pNext; // advance for next layer

        // create instance
        auto* vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
            layer_info->GetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
        if (!vkCreateInstance) {
            std::cerr << "lsfg-vk: failed to get next layer's vkCreateInstance, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        try {
            auto myvk_instance = std::make_unique<MyVkInstance>(myvk_layer,
                *info,
                layer_info->GetInstanceProcAddr,
                [=](VkInstanceCreateInfo* info) {
                    auto res = vkCreateInstance(info, alloc, instance);
                    if (res != VK_SUCCESS)
                        throw ls::vulkan_error(res, "vkCreateInstance() failed");

                    return *instance;
                }
            );
            layer_info->instances.emplace(*instance, std::move(myvk_instance));

            return VK_SUCCESS;
        } catch (const ls::vulkan_error& e) {
            if (e.error() == VK_ERROR_EXTENSION_NOT_PRESENT)
                std::cerr << "lsfg-vk: required Vulkan instance extensions are not present. "
                    "Your GPU driver is not supported.\n";
            else
                std::cerr << "lsfg-vk: something went wrong during lsfg-vk instance initialization:\n"
                    "- " << e.what() << '\n';
            return e.error();
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk instance initialization:\n"
                "- " << e.what() << '\n';
            return VK_ERROR_INITIALIZATION_FAILED;
        }
    }

    // create device
    VkResult myvkCreateDevice(
            VkPhysicalDevice physdev,
            const VkDeviceCreateInfo* info,
            const VkAllocationCallbacks* alloc,
            VkDevice* device) {
        auto& myvk_layer = layer_info->layer;
        auto& myvk_instance = *layer_info->instances.begin()->second;

        // apply layer chaining
        auto* layerInfo = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(info->pNext));
        while (layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
                || layerInfo->function != VK_LAYER_LINK_INFO)) {
            layerInfo = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(layerInfo->pNext));
        }
        if (!layerInfo) {
            std::cerr << "lsfg-vk: no layer info found in pNext chain, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto* linkInfo = layerInfo->u.pLayerInfo;
        if (!linkInfo) {
            std::cerr << "lsfg-vk: link info is null, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        layer_info->GetDeviceProcAddr = linkInfo->pfnNextGetDeviceProcAddr;
        if (!linkInfo->pfnNextGetDeviceProcAddr) {
            std::cerr << "lsfg-vk: next layer's vkGetDeviceProcAddr is null, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        layerInfo->u.pLayerInfo = linkInfo->pNext; // advance for next layer

        // fetch device loader functions
        layerInfo = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(info->pNext));
        while (layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
                || layerInfo->function != VK_LOADER_DATA_CALLBACK)) {
            layerInfo = reinterpret_cast<VkLayerDeviceCreateInfo*>(const_cast<void*>(layerInfo->pNext));
        }
        if (!layerInfo) {
            std::cerr << "lsfg-vk: no layer loader data found in pNext chain.\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto* setLoaderData = layerInfo->u.pfnSetDeviceLoaderData;
        if (!setLoaderData) {
            std::cerr << "lsfg-vk: instance loader data function is null.\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // create device
        auto* vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
            layer_info->GetInstanceProcAddr(myvk_instance.instance(), "vkCreateDevice"));
        if (!vkCreateDevice) {
            std::cerr << "lsfg-vk: failed to get next layer's vkCreateDevice, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        try {
            // Get physical device name for backend GPU selection
            VkPhysicalDeviceProperties2 props{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
            };
            myvk_instance.funcs().GetPhysicalDeviceProperties2(physdev, &props);
            myvk_layer.setActiveGpu(props.properties.deviceName);

            auto myvk_device = std::make_unique<MyVkDevice>(myvk_layer, myvk_instance,
                physdev, *info,
                layer_info->GetDeviceProcAddr, setLoaderData,
                [=](VkDeviceCreateInfo* info) {
                    auto res = vkCreateDevice(physdev, info, alloc, device);
                    if (res != VK_SUCCESS)
                        throw ls::vulkan_error(res, "vkCreateDevice() failed");

                    return *device;
                }
            );
            layer_info->devices.emplace(*device, std::move(myvk_device));

            return VK_SUCCESS;
        } catch (const ls::vulkan_error& e) {
            if (e.error() == VK_ERROR_EXTENSION_NOT_PRESENT)
                std::cerr << "lsfg-vk: required Vulkan device extensions are not present. "
                    "Your GPU driver is not supported.\n";
            else
                std::cerr << "lsfg-vk: something went wrong during lsfg-vk device initialization:\n"
                    "- " << e.what() << '\n';
            return e.error();
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk device initialization:\n"
                "- " << e.what() << '\n';
            return VK_ERROR_INITIALIZATION_FAILED;
        }
    }

    VkResult myvkDeviceWaitIdle(VkDevice device) {
        auto it = layer_info->devices.find(device);
        if (it == layer_info->devices.end())
            return VK_ERROR_DEVICE_LOST;

        const std::scoped_lock<std::mutex> lock(it->second->offload().mutex);
        return it->second->funcs().DeviceWaitIdle(device);
    }

    // destroy device
    void myvkDestroyDevice(VkDevice device, const VkAllocationCallbacks* alloc) {
        layer_info->devices.erase(device);

        auto vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(
            layer_info->GetDeviceProcAddr(device, "vkDestroyDevice"));
        if (!vkDestroyDevice) {
            std::cerr << "lsfg-vk: failed to get next layer's vkDestroyDevice, "
                "the previous layer does not follow spec\n";
            return;
        }

        vkDestroyDevice(device, alloc);
    }

    // destroy instance
    void myvkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* alloc) {
        layer_info->instances.erase(instance);

        auto vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(
            layer_info->GetInstanceProcAddr(instance, "vkDestroyInstance"));
        if (!vkDestroyInstance) {
            std::cerr << "lsfg-vk: failed to get next layer's vkDestroyInstance, "
                "the previous layer does not follow spec\n";
            return;
        }

        vkDestroyInstance(instance, alloc);
    }

    // get optional function pointer override
    PFN_vkVoidFunction getProcAddr(const std::string& name) {
        auto it = layer_info->map.find(name);
        if (it != layer_info->map.end())
            return it->second;
        return nullptr;
    }

    // get instance-level function pointers
    PFN_vkVoidFunction myvkGetInstanceProcAddr(VkInstance instance, const char* name) {
        if (!name) return nullptr;

        auto func = getProcAddr(name);
        if (func) return func;

        if (!layer_info->GetInstanceProcAddr) return nullptr;
        return layer_info->GetInstanceProcAddr(instance, name);
    }

    // get device-level function pointers
    PFN_vkVoidFunction myvkGetDeviceProcAddr(VkDevice device, const char* name) {
        if (!name) return nullptr;

        auto func = getProcAddr(name);
        if (func) return func;

        if (!layer_info->GetDeviceProcAddr) return nullptr;
        return layer_info->GetDeviceProcAddr(device, name);
    }
}

namespace {
    VkResult myvkCreateSwapchainKHR(
            VkDevice device,
            const VkSwapchainCreateInfoKHR* info,
            const VkAllocationCallbacks* alloc,
            VkSwapchainKHR* swapchain) {
        auto& myvk_layer = layer_info->layer;
        auto& myvk_instance = *layer_info->instances.begin()->second;
        auto& myvk_device = *layer_info->devices.find(device)->second;

        auto* vkCreateSwapchainKHR = myvk_device.funcs().CreateSwapchainKHR;
        layer_info->QueueSubmit = myvk_device.funcs().QueueSubmit;

        try {
            myvk_layer.update(); // ensure config is up to date

            // remove old managed swapchain
            if (info->oldSwapchain)
                layer_info->swapchains.erase(info->oldSwapchain);

            // create managed swapchain
            auto myvk_swapchain = std::make_unique<MyVkSwapchain>(myvk_layer, myvk_instance, myvk_device,
                *info,
                [=](VkSwapchainCreateInfoKHR* info) {
                    auto res = vkCreateSwapchainKHR(device, info, alloc, swapchain);
                    if (res != VK_SUCCESS)
                        throw ls::vulkan_error(res, "vkCreateSwapchainKHR() failed");

                    return *swapchain;
                }
            );
            layer_info->swapchains.emplace(*swapchain, std::move(myvk_swapchain));

            return VK_SUCCESS;
        } catch (const ls::vulkan_error& e) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk swapchain creation:\n"
                "- " << e.what() << '\n';
            return e.error();
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk swapchain creation:\n"
                "- " << e.what() << '\n';
            return VK_ERROR_UNKNOWN;
        }
    }

    VkResult myvkGetSwapchainImagesKHR(
            VkDevice,
            VkSwapchainKHR swapchain,
            uint32_t* count,
            VkImage* images) {
        const auto& it = layer_info->swapchains.find(swapchain);
        if (it == layer_info->swapchains.end())
            return VK_ERROR_SURFACE_LOST_KHR;

        return it->second->GetSwapchainImagesKHR(count, images);
    }

    VkResult myvkAcquireNextImageKHR(
            VkDevice,
            VkSwapchainKHR swapchain,
            uint64_t timeout,
            VkSemaphore semaphore,
            VkFence fence,
            uint32_t* idx) {
        const auto& it = layer_info->swapchains.find(swapchain);
        if (it == layer_info->swapchains.end())
            return VK_ERROR_SURFACE_LOST_KHR;

        return it->second->AcquireNextImageKHR(timeout, semaphore, fence, idx);
    }

    VkResult myvkAcquireNextImage2KHR(
            VkDevice,
            const VkAcquireNextImageInfoKHR* info,
            uint32_t* idx) {
        const auto& it = layer_info->swapchains.find(info->swapchain);
        if (it == layer_info->swapchains.end())
            return VK_ERROR_SURFACE_LOST_KHR;

        return it->second->AcquireNextImage2KHR(info, idx);
    }

    VkResult myvkReleaseSwapchainImagesKHR(
            VkDevice,
            const VkReleaseSwapchainImagesInfoKHR* info) {
        const auto& it = layer_info->swapchains.find(info->swapchain);
        if (it == layer_info->swapchains.end())
            return VK_ERROR_SURFACE_LOST_KHR;

        return it->second->ReleaseSwapchainImagesKHR(info);
    }

    VkResult myvkWaitForPresentKHR(
            VkDevice,
            VkSwapchainKHR swapchain,
            uint64_t id,
            uint64_t timeout) {
        const auto& it = layer_info->swapchains.find(swapchain);
        if (it == layer_info->swapchains.end())
            return VK_ERROR_SURFACE_LOST_KHR;

        return it->second->WaitForPresentKHR(id, timeout);
    }

    VkResult myvkWaitForPresent2KHR(
            VkDevice,
            VkSwapchainKHR swapchain,
            const VkPresentWait2InfoKHR* info) {
        const auto& it = layer_info->swapchains.find(swapchain);
        if (it == layer_info->swapchains.end())
            return VK_ERROR_SURFACE_LOST_KHR;

        return it->second->WaitForPresent2KHR(info);
    }

    VkResult myvkQueuePresentKHR(VkQueue queue,
            const VkPresentInfoKHR* info) {
        VkResult result = VK_SUCCESS;

        // re-create out-of-date managed swapchains
        try {
            if (layer_info->layer.update()) {
                for (auto& [handle, myswapchain] : layer_info->swapchains)
                    myswapchain->reinitialize();
            }
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk swapchain re-creation:\n"
                "- " << e.what() << '\n';
            // ignore error: return VK_ERROR_UNKNOWN;
        }

        // check for VK_KHR_swapchain_maintenance1
        std::vector<VkFence> fences(info->swapchainCount, VK_NULL_HANDLE);
        std::vector<std::optional<VkPresentModeKHR>> modes(info->swapchainCount, std::nullopt);

        const auto* fenceInfo = ls::find_structure<VkSwapchainPresentFenceInfoKHR>(
            VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR,
            info->pNext
        );
        if (fenceInfo) {
            for (uint32_t i = 0; i < fenceInfo->swapchainCount; i++)
                fences[i] = fenceInfo->pFences[i];
        }

        const auto* presentModeInfo = ls::find_structure<VkSwapchainPresentModeInfoKHR>(
            VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_KHR,
            info->pNext
        );
        if (presentModeInfo) {
            for (uint32_t i = 0; i < presentModeInfo->swapchainCount; i++)
                modes[i] = presentModeInfo->pPresentModes[i];
        }

        // check for VK_KHR_present_id(2)
        std::vector<std::optional<uint64_t>> presentIds(info->swapchainCount, std::nullopt);

        const auto* presentIdInfo = ls::find_structure<VkPresentIdKHR>(
            VK_STRUCTURE_TYPE_PRESENT_ID_KHR,
            info->pNext
        );
        if (presentIdInfo) {
            for (uint32_t i = 0; i < presentIdInfo->swapchainCount; i++)
                presentIds[i] = presentIdInfo->pPresentIds[i];
        }

        const auto* presentId2Info = ls::find_structure<VkPresentId2KHR>(
            VK_STRUCTURE_TYPE_PRESENT_ID_2_KHR,
            info->pNext
        );
        if (presentId2Info) {
            for (uint32_t i = 0; i < presentId2Info->swapchainCount; i++)
                presentIds[i] = presentId2Info->pPresentIds[i];
        }

        // collect semaphores and values
        std::vector<uint64_t> waitValues(info->waitSemaphoreCount);
        std::vector<VkSemaphore> signalSemaphores;
        std::vector<uint64_t> signalValues;

        for (uint32_t i = 0; i < info->swapchainCount; i++) {
            const auto& handle = info->pSwapchains[i];

            const auto& it = layer_info->swapchains.find(handle);
            if (it == layer_info->swapchains.end())
                return VK_ERROR_SURFACE_LOST_KHR;

            const auto& sync = it->second->sync();
            signalSemaphores.push_back(sync.first);
            signalValues.push_back(sync.second);
        }

        // present all managed swapchains
        for (uint32_t i = 0; i < info->swapchainCount; i++) {
            const auto& handle = info->pSwapchains[i];

            const auto& it = layer_info->swapchains.find(handle);
            if (it == layer_info->swapchains.end())
                return VK_ERROR_SURFACE_LOST_KHR;

            try {
                const auto& fence = fences[i];
                const auto& present_mode = modes[i];
                const auto& present_id = presentIds[i];

                const MyVkPresentInfo mypresentinfo{
                    .idx = info->pImageIndices[i],
                    .fence = fence,
                    .present_mode = present_mode,
                    .id = present_id
                };
                result = it->second->partial_QueuePresentKHR(mypresentinfo);
            } catch (const ls::vulkan_error& e) {
                if (e.error() != VK_ERROR_OUT_OF_DATE_KHR) { // swallow out-of-date errors
                    std::cerr << "lsfg-vk: something went wrong during lsfg-vk swapchain presentation:\n"
                        "- " << e.what() << '\n';
                }
                result = e.error();
            } catch (const std::exception& e) {
                std::cerr << "lsfg-vk: something went wrong during lsfg-vk swapchain presentation:\n"
                    "- " << e.what() << '\n';
                result = VK_ERROR_UNKNOWN;
            }

            if (result != VK_SUCCESS && info->pResults)
                info->pResults[i] = result;
        }

        // submit the present operation
        const VkTimelineSemaphoreSubmitInfo timelineInfo{
            .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size()),
            .pWaitSemaphoreValues = waitValues.data(),
            .signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size()),
            .pSignalSemaphoreValues = signalValues.data()
        };
        std::vector<VkPipelineStageFlags> stages(info->waitSemaphoreCount,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        const VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = &timelineInfo,
            .waitSemaphoreCount = info->waitSemaphoreCount,
            .pWaitSemaphores = info->pWaitSemaphores,
            .pWaitDstStageMask = stages.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphores = signalSemaphores.data()
        };
        auto res = layer_info->QueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        if (res != VK_SUCCESS) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk present submission:\n"
                "- vkQueueSubmit() failed with error " << res << '\n';
            return res;
        }

        return result;
    }

    void myvkDestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* alloc) {
        layer_info->swapchains.erase(swapchain);

        auto it = layer_info->devices.find(device);
        if (it == layer_info->devices.end())
            return;

        auto vkDestroySwapchainKHR = it->second->funcs().DestroySwapchainKHR;
        vkDestroySwapchainKHR(device, swapchain, alloc);
    }
}

namespace {

}

/// Vulkan layer entrypoint
__attribute__((visibility("default")))
VkResult vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* pVersionStruct) {
    // ensure loader compatibility
    if (!pVersionStruct
        || pVersionStruct->sType != LAYER_NEGOTIATE_INTERFACE_STRUCT
        || pVersionStruct->loaderLayerInterfaceVersion < 2)
        return VK_ERROR_INITIALIZATION_FAILED;

    // if the layer has already been initialized, skip
    if (layer_info) {
        pVersionStruct->loaderLayerInterfaceVersion = 2;
        pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
        pVersionStruct->pfnGetDeviceProcAddr = myvkGetDeviceProcAddr;
        pVersionStruct->pfnGetInstanceProcAddr = myvkGetInstanceProcAddr;
        return VK_SUCCESS;
    }

    // load the layer configuration
    try {
        layer_info = new LayerInfo { // NOLINT (memory management)
            .map = {
#define VKPTR(name) reinterpret_cast<PFN_vkVoidFunction>(name)
                { "vkCreateInstance", VKPTR(myvkCreateInstance) },
                { "vkCreateDevice", VKPTR(myvkCreateDevice) },
                { "vkDeviceWaitIdle", VKPTR(myvkDeviceWaitIdle) },
                { "vkDestroyDevice", VKPTR(myvkDestroyDevice) },
                { "vkDestroyInstance", VKPTR(myvkDestroyInstance) },
                { "vkCreateSwapchainKHR", VKPTR(myvkCreateSwapchainKHR) },
                { "vkGetSwapchainImagesKHR", VKPTR(myvkGetSwapchainImagesKHR) },
                { "vkAcquireNextImageKHR", VKPTR(myvkAcquireNextImageKHR) },
                { "vkAcquireNextImage2KHR", VKPTR(myvkAcquireNextImage2KHR) },
                { "vkReleaseSwapchainImagesKHR", VKPTR(myvkReleaseSwapchainImagesKHR) },
                { "vkReleaseSwapchainImagesEXT", VKPTR(myvkReleaseSwapchainImagesKHR) },
                { "vkWaitForPresentKHR", VKPTR(myvkWaitForPresentKHR) },
                { "vkWaitForPresent2KHR", VKPTR(myvkWaitForPresent2KHR) },
                { "vkQueuePresentKHR", VKPTR(myvkQueuePresentKHR) },
                { "vkDestroySwapchainKHR", VKPTR(myvkDestroySwapchainKHR) }
#undef VKPTR
            }
        };

        if (!layer_info->layer.isActive()) { // skip inactive
            delete layer_info; // NOLINT (memory management)
            layer_info = nullptr;

            return VK_ERROR_INITIALIZATION_FAILED;
        }
    } catch (const std::exception& e) {
        std::cerr << "lsfg-vk: something went wrong during lsfg-vk layer initialization:\n"
            "- " << e.what() << '\n';

        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // emplace function pointers/version
    pVersionStruct->loaderLayerInterfaceVersion = 2;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
    pVersionStruct->pfnGetDeviceProcAddr = myvkGetDeviceProcAddr;
    pVersionStruct->pfnGetInstanceProcAddr = myvkGetInstanceProcAddr;
    return VK_SUCCESS;
}
