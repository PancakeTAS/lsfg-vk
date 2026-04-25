/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "vkhelper.hpp"

#include <array>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

/* Device initialization */

vk::UniqueInstance vkhelper::createInstance(vk::detail::DispatchLoaderDynamic& dld) {
    dld.init();

    const vk::ApplicationInfo appInfo{
        .pApplicationName = "lsfg-vk",
        .applicationVersion = vk::makeVersion(2, 0, 0),
        .pEngineName = "lsfg-vk",
        .engineVersion = vk::makeVersion(2, 0, 0),
        .apiVersion = vk::ApiVersion12 // Fully supported by all Vulkan-capable GPUs
    };
    const vk::InstanceCreateInfo instanceInfo{
        .pApplicationInfo = &appInfo
    };
    auto instance{vk::createInstanceUnique(instanceInfo, nullptr, dld)};
    dld.init(*instance);

    return instance;
}

vk::PhysicalDevice vkhelper::findPhysicalDevice(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Instance& instance,
    const std::string& id
) {
    for (const auto& physdev : instance.enumeratePhysicalDevices(dld)) {
        // Check for VK_EXT_pci_bus_info
        bool supportsPCIEXT{false};
        for (const auto& ext : physdev.enumerateDeviceExtensionProperties(nullptr, dld)) {
            if (std::string(ext.extensionName) != vk::EXTPciBusInfoExtensionName)
                continue;

            supportsPCIEXT = true;
            break;
        }

        // Fetch properties
        vk::PhysicalDevicePCIBusInfoPropertiesEXT busInfo{};
        vk::PhysicalDeviceProperties2 info{
            .pNext = supportsPCIEXT ? &busInfo : nullptr
        };
        physdev.getProperties2(&info, dld);

        auto& props{info.properties};

        // Compare device name
        props.deviceName.back() = '\0'; // Ensure null-termination
        if (id == std::string(props.deviceName))
            return physdev;

        // Compare Vendor ID + Device ID
        std::ostringstream gpuss;
        gpuss << std::hex << std::setfill('0')
            << std::setw(4) << props.vendorID << ":"
            << std::setw(4) << props.deviceID;
        if (id == gpuss.str())
            return physdev;

        // Compare PCI bus ID
        if (!supportsPCIEXT)
            continue;

        std::ostringstream pciss;
        pciss << std::hex << std::setfill('0')
            << std::setw(4) << busInfo.pciDomain << ":"
            << std::setw(2) << busInfo.pciBus << ":"
            << std::setw(2) << busInfo.pciDevice << "."
            << std::setw(1) << busInfo.pciFunction;
        if (id == pciss.str())
            return physdev;
    }

    throw std::runtime_error("No physical device matching '" + id + "' found");
}

uint32_t vkhelper::findComputeQueueFamilyIndex(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::PhysicalDevice& physdev
) {
    uint32_t idx{0};
    for (const auto& qfi : physdev.getQueueFamilyProperties2(dld)) {
        if (qfi.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute)
            return idx;
        idx++;
    }

    throw std::runtime_error("No compute-capable queue family found");
}

bool vkhelper::checkHalfPrecisionSupport(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::PhysicalDevice& physdev
) {
    vk::PhysicalDeviceVulkan12Features featuresVulkan12{};
    vk::PhysicalDeviceFeatures2 features{
        .pNext = &featuresVulkan12
    };
    physdev.getFeatures2(&features, dld);
    return featuresVulkan12.shaderFloat16;
}

std::pair<vk::UniqueDevice, vk::Queue> vkhelper::createDevice(
    vk::detail::DispatchLoaderDynamic& dld,
    const vk::PhysicalDevice& physdev,
    uint32_t qfi,
    bool fp16
) {
    constexpr std::array<const char*, 3> EXTENSIONS{
        vk::KHRSynchronization2ExtensionName,
        vk::KHRExternalMemoryFdExtensionName,
        vk::KHRExternalSemaphoreFdExtensionName
    };

    vk::PhysicalDeviceSynchronization2FeaturesKHR sync2Info{
        .synchronization2 = VK_TRUE
    };
    const vk::PhysicalDeviceVulkan12Features vk12Info{
        .pNext = &sync2Info,
        .shaderFloat16 = fp16,
        .timelineSemaphore = VK_TRUE
    };
    const float queuePriority{1.0F}; // Highest priority
    const vk::DeviceQueueCreateInfo queueInfo{
        .queueFamilyIndex = qfi,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    const vk::DeviceCreateInfo deviceInfo{
        .pNext = &vk12Info,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(EXTENSIONS.size()),
        .ppEnabledExtensionNames = EXTENSIONS.data()
    };
    auto device{physdev.createDeviceUnique(deviceInfo, nullptr, dld)};
    dld.init(*device);

    return{
        std::move(device),
        device->getQueue(qfi, 0, dld)
    };
}

/* Shader modules & pipelines */

vk::UniqueShaderModule vkhelper::createShaderModule(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const std::span<const uint32_t>& code
) {
    const vk::ShaderModuleCreateInfo shaderInfo{
        .codeSize = code.size() * sizeof(uint32_t),
        .pCode = code.data()
    };
    return device.createShaderModuleUnique(shaderInfo, nullptr, dld);
}
