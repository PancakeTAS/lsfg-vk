#include "context/instance.hpp"
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
}

namespace {
    // global layer info initialized at layer negotiation
    struct LayerInfo {
        std::unordered_map<std::string, PFN_vkVoidFunction> map; //!< function pointer override map
        PFN_vkGetInstanceProcAddr GetInstanceProcAddr;

        layer::Root root;
    }* layer_info;

    // instance-wide info initialized at instance creation
    struct InstanceInfo {
        VkInstance handle;
        vk::VulkanInstanceFuncs funcs;

        std::unordered_map<VkDevice, vk::Vulkan> devices;
    }* instance_info;

    // create instance
    VkResult myvkCreateInstance(
            const VkInstanceCreateInfo* info,
            const VkAllocationCallbacks* alloc,
            VkInstance* instance) {
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
            layer_info->GetInstanceProcAddr(nullptr, "vkCreateInstance"));
        if (!vkCreateInstance) {
            std::cerr << "lsfg-vk: failed to get next layer's vkCreateInstance, "
                "the previous layer does not follow spec\n";
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto extensions = add_extensions(
            info->ppEnabledExtensionNames,
            info->enabledExtensionCount,
            layer_info->root.instanceExtensions());

        VkInstanceCreateInfo newInfo = *info;
        newInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        newInfo.ppEnabledExtensionNames = extensions.data();

        auto res = vkCreateInstance(&newInfo, alloc, instance);
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
            std::cerr << "lsfg-vk: required Vulkan instance extensions are not present. "
                "Your GPU driver is not supported.\n";

        if (res != VK_SUCCESS)
            return res;

        instance_info = new InstanceInfo{
            .handle = *instance,
            .funcs = vk::initVulkanInstanceFuncs(*instance, layer_info->GetInstanceProcAddr),
            .devices = {}
        };
        return VK_SUCCESS;
    }

    // create device
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

        instance_info->funcs.GetDeviceProcAddr = linkInfo->pfnNextGetDeviceProcAddr;
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
            layer_info->root.deviceExtensions());

        VkDeviceCreateInfo newInfo = *info;
        newInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        newInfo.ppEnabledExtensionNames = extensions.data();

        auto res = instance_info->funcs.CreateDevice(physdev, &newInfo, alloc, device);
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
            std::cerr << "lsfg-vk: required Vulkan device extensions are not present. "
                "Your GPU driver is not supported.\n";

        if (res != VK_SUCCESS)
            return res;

        // create layer instance
        try {
            instance_info->devices.emplace(
                *device,
                vk::Vulkan(
                    instance_info->handle, *device, physdev,
                    instance_info->funcs, vk::initVulkanDeviceFuncs(instance_info->funcs, *device),
                    true, setLoaderData
                )
            );
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: something went wrong during lsfg-vk initialization:\n";
            std::cerr << "- " << e.what() << '\n';
        }

        return VK_SUCCESS;
    }

    // destroy device
    void myvkDestroyDevice(VkDevice device, const VkAllocationCallbacks* alloc) {
        // destroy layer instance
        auto it = instance_info->devices.find(device);
        if (it != instance_info->devices.end())
            instance_info->devices.erase(it);

        // destroy device
        auto vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(
            instance_info->funcs.GetDeviceProcAddr(device, "vkDestroyDevice"));
        if (!vkDestroyDevice) {
            std::cerr << "lsfg-vk: failed to get next layer's vkDestroyDevice, "
                "the previous layer does not follow spec\n";
            return;
        }

        vkDestroyDevice(device, alloc);
    }

    // destroy instance
    void myvkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* alloc) {
        // destroy instance info
        delete instance_info;
        instance_info = nullptr;

        // destroy instance
        auto vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(
            layer_info->GetInstanceProcAddr(instance, "vkDestroyInstance"));
        if (!vkDestroyInstance) {
            std::cerr << "lsfg-vk: failed to get next layer's vkDestroyInstance, "
                "the previous layer does not follow spec\n";
            return;
        }

        vkDestroyInstance(instance, alloc);

        // destroy layer info
        // NOTE: there's no real way of unloading the layer without a deconstructor.
        // multiple instances just aren't common enough to worry about it.
        // NOTE2: it doesn't really matter anyways, because the myvkDestroyDevice code
        // freezes the entire thing anyways.
        delete layer_info;
        layer_info = nullptr;
    }

    // get optional function pointer override
    PFN_vkVoidFunction getProcAddr(const std::string& name) {
        auto it = layer_info->map.find(name);
        if (it != layer_info->map.end())
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

        if (!layer_info->GetInstanceProcAddr) return nullptr;
        return layer_info->GetInstanceProcAddr(instance, pName);
    }

    // get device-level function pointers
    PFN_vkVoidFunction myvkGetDeviceProcAddr(VkDevice device, const char* pName) {
        if (!pName) return nullptr;

        auto func = getProcAddr(pName);
        if (func) return func;

        if (!instance_info->funcs.GetDeviceProcAddr) return nullptr;
        return instance_info->funcs.GetDeviceProcAddr(device, pName);
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

    // load the layer configuration
    try {
        layer_info = new LayerInfo {
            .map = {
#define VKPTR(name) reinterpret_cast<PFN_vkVoidFunction>(name)
                { "vkCreateInstance", VKPTR(myvkCreateInstance) },
                { "vkCreateDevice", VKPTR(myvkCreateDevice) },
                { "vkDestroyDevice", VKPTR(myvkDestroyDevice) },
                { "vkDestroyInstance", VKPTR(myvkDestroyInstance) }
#undef VKPTR
            },
            .root = layer::Root()
        };

        if (!layer_info->root.active()) { // skip inactive
            delete layer_info;
            layer_info = nullptr;

            return VK_ERROR_INITIALIZATION_FAILED;
        }
    } catch (const std::exception& e) {
        std::cerr << "lsfg-vk: something went wrong during lsfg-vk layer initialization:\n";
        std::cerr << "- " << e.what() << '\n';

        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // emplace function pointers/version
    pVersionStruct->loaderLayerInterfaceVersion = 2;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
    pVersionStruct->pfnGetDeviceProcAddr = myvkGetDeviceProcAddr;
    pVersionStruct->pfnGetInstanceProcAddr = myvkGetInstanceProcAddr;
    return VK_SUCCESS;
}
