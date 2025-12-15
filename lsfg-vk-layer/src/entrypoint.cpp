#include "layer.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk;

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

    /// struct containing various helper globals
    struct Globals {
        layer::Layer layer;

        VkInstance instance{VK_NULL_HANDLE}; // if there are multiple instances, we scream
        vk::VulkanInstanceFuncs fi{};

        std::unordered_map<std::string, PFN_vkVoidFunction> procAddrMap; // all overriden pointers

        std::unordered_map<VkDevice, layer::LayerInstance> device2InstanceMap;
    };
}

namespace {
    Globals* global{nullptr};
    void populateProcAddrMap();

    // create instance
    PFN_vkGetInstanceProcAddr nxvkGetInstanceProcAddr{nullptr};
    VkResult myvkCreateInstance(
            const VkInstanceCreateInfo* info,
            const VkAllocationCallbacks* alloc,
            VkInstance* instance) {
        // try to load lsfg-vk layer
        try {
            if (!global) { // cleanup in vkDestroyInstance
                global = new Globals();
                populateProcAddrMap();
            }
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk layer initialization:\n";
            std::cerr << "- " << e.what() << '\n';
        }

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

        nxvkGetInstanceProcAddr = linkInfo->pfnNextGetInstanceProcAddr;
        if (!nxvkGetInstanceProcAddr) {
            std::cerr << "lsfg-vk: next layer's vkGetInstanceProcAddr is null, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        layerInfo->u.pLayerInfo = linkInfo->pNext; // advance for next layer

        // create instance
        auto* vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
            nxvkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
        if (!vkCreateInstance) {
            std::cerr << "lsfg-vk: failed to get next layer's vkCreateInstance, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        const auto& l = global;
        if (!l || !l->layer.active()) // skip inactive
            return vkCreateInstance(info, alloc, instance);

        auto extensions = add_extensions(
            info->ppEnabledExtensionNames,
            info->enabledExtensionCount,
            l->layer.instanceExtensions());

        VkInstanceCreateInfo newInfo = *info;
        newInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        newInfo.ppEnabledExtensionNames = extensions.data();

        auto res = vkCreateInstance(&newInfo, alloc, instance);
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
            std::cerr << "lsfg-vk: required Vulkan instance extensions are not present. "
                "Your GPU driver is not supported.\n";

        if (res != VK_SUCCESS)
            return res;

        l->instance = *instance;
        l->fi = vk::initVulkanInstanceFuncs(l->instance, nxvkGetInstanceProcAddr);
        return VK_SUCCESS;
    }

    // create device
    PFN_vkGetDeviceProcAddr nxvkGetDeviceProcAddr{nullptr};
    VkResult myvkCreateDevice(
            VkPhysicalDevice physdev,
            const VkDeviceCreateInfo* info,
            const VkAllocationCallbacks* alloc,
            VkDevice* device) {
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

        global->fi.GetDeviceProcAddr = linkInfo->pfnNextGetDeviceProcAddr;
        nxvkGetDeviceProcAddr = linkInfo->pfnNextGetDeviceProcAddr;
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
        auto extensions = add_extensions(
            info->ppEnabledExtensionNames,
            info->enabledExtensionCount,
            global->layer.deviceExtensions());

        VkDeviceCreateInfo newInfo = *info;
        newInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        newInfo.ppEnabledExtensionNames = extensions.data();

        auto res = global->fi.CreateDevice(physdev, &newInfo, alloc, device);
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
            std::cerr << "lsfg-vk: required Vulkan device extensions are not present. "
                "Your GPU driver is not supported.\n";

        if (res != VK_SUCCESS)
            return res;

        // create layer instance
        try {
            global->device2InstanceMap.emplace(
                *device,
                layer::LayerInstance(
                    global->layer,
                    vk::Vulkan(
                        global->instance, *device,
                        physdev,
                        global->fi,
                        vk::initVulkanDeviceFuncs(global->fi, *device),
                        setLoaderData
                    )
                )
            );
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk initialization:\n";
            std::cerr << "- " << e.what() << '\n';
        }

        return VK_SUCCESS;
    }

    // populate function pointer override map
    void populateProcAddrMap() {
#define VKPTR(name) reinterpret_cast<PFN_vkVoidFunction>(name)
        global->procAddrMap = {
            { "vkCreateInstance", VKPTR(myvkCreateInstance) },
            { "vkCreateDevice", VKPTR(myvkCreateDevice) },
        };
#undef VKPTR
    }

    // get optional function pointer override
    PFN_vkVoidFunction getProcAddr(const std::string& name) {
        if (!global) return nullptr;
        auto it = global->procAddrMap.find(name);
        if (it != global->procAddrMap.end())
            return it->second;
        return nullptr;
    }

    // get instance-level function pointers
    PFN_vkVoidFunction myvkGetInstanceProcAddr(VkInstance instance, const char* pName) {
        if (!pName) return nullptr;

        if (std::string(pName) == "vkCreateInstance") // pre-instance function
            return reinterpret_cast<PFN_vkVoidFunction>(myvkCreateInstance);

        auto func = getProcAddr(pName);
        if (func) return func;

        if (!nxvkGetInstanceProcAddr) return nullptr;
        return nxvkGetInstanceProcAddr(instance, pName);
    }

    // get device-level function pointers
    PFN_vkVoidFunction myvkGetDeviceProcAddr(VkDevice device, const char* pName) {
        if (!pName) return nullptr;

        auto func = getProcAddr(pName);
        if (func) return func;

        if (!nxvkGetDeviceProcAddr) return nullptr;
        return nxvkGetDeviceProcAddr(device, pName);
    }

}

/// Vulkan layer entrypoint
__attribute__((visibility("default")))
VkResult vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* pVersionStruct) {
    // ensure loader compatibility
    if (!pVersionStruct
        || pVersionStruct->sType != LAYER_NEGOTIATE_INTERFACE_STRUCT
        || pVersionStruct->loaderLayerInterfaceVersion < 2)
        return VK_ERROR_INITIALIZATION_FAILED;

    // emplace function pointers/version
    pVersionStruct->loaderLayerInterfaceVersion = 2;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
    pVersionStruct->pfnGetDeviceProcAddr = myvkGetDeviceProcAddr;
    pVersionStruct->pfnGetInstanceProcAddr = myvkGetInstanceProcAddr;
    return VK_SUCCESS;
}
