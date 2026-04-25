/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "vkhelper.hpp"

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

std::pair<vk::UniqueDescriptorSetLayout, vk::UniquePipelineLayout> vkhelper::createLayout(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
    size_t pushConstantSize
) {
    const vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    auto descriptorSetLayout{device.createDescriptorSetLayoutUnique(layoutInfo, nullptr, dld)};

    const vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .size = static_cast<uint32_t>(pushConstantSize)
    };
    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    auto pipelineLayout{device.createPipelineLayoutUnique(pipelineLayoutInfo, nullptr, dld)};

    return { std::move(descriptorSetLayout), std::move(pipelineLayout) };
}

/* Resources */

vk::UniqueImage vkhelper::createImage(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    vk::Extent2D extent,
    vk::Format format,
    uint32_t layers,
    vk::ImageUsageFlags usage
) {
    const vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = layers,
        .samples = vk::SampleCountFlagBits::e1,
        .usage = usage
    };
    return device.createImageUnique(imageInfo, nullptr, dld);
}

/* External memory */

std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> vkhelper::createExternalImage(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::PhysicalDevice& physdev,
    vk::Extent2D extent,
    vk::Format format,
    uint32_t layers,
    vk::ImageUsageFlags usage
) {
    const vk::ExternalMemoryImageCreateInfo externalInfo{
        .handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd
    };
    const vk::ImageCreateInfo imageInfo{
        .pNext = &externalInfo,
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = layers,
        .samples = vk::SampleCountFlagBits::e1,
        .usage = usage
    };
    auto image{device.createImageUnique(imageInfo, nullptr, dld)};

    // Find a suitable memory type index
    const auto memProps{physdev.getMemoryProperties2(dld)};
    const auto requirements{device.getImageMemoryRequirements(*image, dld)};

    std::optional<uint32_t> selectedTypeIdx{};
    for (uint32_t i = 0; i < memProps.memoryProperties.memoryTypeCount; i++) {
        if (!std::bitset<32>(requirements.memoryTypeBits).test(i))
            continue;
        const auto& memType{memProps.memoryProperties.memoryTypes.at(i)};

        if (memType.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
            selectedTypeIdx = i;
            break;
        }
    }

    if (!selectedTypeIdx)
        throw std::runtime_error("No suitable memory type found for allocation");

    // Allocate memory
    const vk::MemoryDedicatedAllocateInfo dedicatedInfo{
        .image = *image,
    };
    const vk::ExportMemoryAllocateInfo exportInfo{
        .pNext = &dedicatedInfo,
        .handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd
    };
    const vk::MemoryAllocateInfo allocInfo{
        .pNext = &exportInfo,
        .allocationSize = requirements.size,
        .memoryTypeIndex = *selectedTypeIdx
    };
    auto memory{device.allocateMemoryUnique(allocInfo, nullptr, dld)};

    // Bind memory
    device.bindImageMemory(*image, *memory, 0, dld);

    return{
        std::move(image),
        std::move(memory)
    };
}
