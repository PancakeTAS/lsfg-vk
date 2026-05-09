/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "vkhelper.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
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

        // Check first if id is not given
        if (id.empty())
            return physdev;

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

namespace {
    /// Find the cache file path
    std::filesystem::path findPipelineCache(
        const vk::detail::DispatchLoaderDynamic& dld,
        const vk::PhysicalDevice& physdev,
        std::string_view tag
    ) {
        // First find the base path
        std::filesystem::path path{"/tmp/lsfg-vk"};

        const char* xdgCacheHome{std::getenv("XDG_CACHE_HOME")};
        if (xdgCacheHome && *xdgCacheHome != '\0')
            path = std::filesystem::path(xdgCacheHome) / "lsfg-vk";

        const char* home{std::getenv("HOME")};
        if (home && *home != '\0')
            path = std::filesystem::path(home) / ".cache" / "lsfg-vk";

        // Ensure the directory exists
        if (!std::filesystem::exists(path))
            std::filesystem::create_directories(path);

        // Calculate the physical device UUID
        vk::PhysicalDeviceProperties2 info{};
        physdev.getProperties2(&info, dld);

        std::ostringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint32_t i = 0; i < 16; i++) {
            ss << std::setw(2) << static_cast<uint32_t>(info.properties.pipelineCacheUUID.at(i));
            if (i == 3 || i == 5 || i == 7 || i == 9) {
                ss << "-";
            }
        }

        // Return the full path
        return path / ("cache_" + std::string(tag) + "_" + ss.str() + ".bin");
    }
}

std::pair<vk::UniquePipelineCache, bool> vkhelper::createPipelineCache(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::PhysicalDevice& physdev,
    std::string_view tag
) {
    const std::filesystem::path path{findPipelineCache(dld, physdev, tag)};
    const bool valid{std::filesystem::exists(path) && std::filesystem::file_size(path) > 32};

    // Read cache data (if any)
    std::vector<uint8_t> cacheData{};
    if (std::filesystem::exists(path)) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw std::runtime_error("Unable to open pipeline cache file for reading");

        const std::streamsize size{static_cast<std::streamsize>(file.tellg())};
        cacheData = std::vector<uint8_t>(static_cast<size_t>(size));

        file.seekg(0, std::ios::beg);
        if (!file.read(reinterpret_cast<char*>(cacheData.data()), size)) // NOLINT (unsafe cast)
            throw std::runtime_error("Unable to read pipeline cache file");
    }

    // Build pipeline cache
    const vk::PipelineCacheCreateInfo pipelineCacheInfo{
        .initialDataSize = cacheData.size(),
        .pInitialData = cacheData.data()
    };
    return { device.createPipelineCacheUnique(pipelineCacheInfo, nullptr, dld), valid };
}

void vkhelper::persistPipelineCache(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::PhysicalDevice& physdev,
    const vk::PipelineCache& cache,
    std::string_view tag
) {
    const std::filesystem::path path{findPipelineCache(dld, physdev, tag)};

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        throw std::runtime_error("Unable to open pipeline cache file for writing");

    const std::vector<uint8_t> cacheData{
        device.getPipelineCacheData(cache, dld)
    };
    file.write(
        reinterpret_cast<const char*>(cacheData.data()), // NOLINT (unsafe cast)
        static_cast<std::streamsize>(cacheData.size())
    );

    file.flush();
    file.close();
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

vk::UniqueSampler vkhelper::createSampler(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    vk::SamplerAddressMode mode,
    vk::CompareOp compare,
    bool white
) {
    const vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = mode,
        .addressModeV = mode,
        .addressModeW = mode,
        .compareOp = compare,
        .maxLod = vk::LodClampNone,
        .borderColor = white ?
            vk::BorderColor::eFloatOpaqueWhite : vk::BorderColor::eFloatTransparentBlack
    };
    return device.createSamplerUnique(samplerInfo, nullptr, dld);
}

std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> vkhelper::createBuffer(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::PhysicalDevice& physdev,
    vk::BufferUsageFlags usage,
    const void* data,
    size_t size
) {
    // Create buffer
    const vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };
    auto buffer{device.createBufferUnique(bufferInfo, nullptr, dld)};

    // Allocate memory
    const auto requirements{device.getBufferMemoryRequirements(*buffer, dld)};

    auto memory{vkhelper::allocateMemory(
        dld,
        device,
        physdev,
        requirements.size,
        requirements.memoryTypeBits,
        true
    )};

    // Bind memory
    device.bindBufferMemory(*buffer, *memory, 0, dld);

    // Copy data
    if (data) {
        void* mapped{device.mapMemory(*memory, 0, size, {}, dld)};
        std::copy_n(
            reinterpret_cast<const uint8_t*>(data), // NOLINT (unsafe cast)
            size,
            reinterpret_cast<uint8_t*>(mapped) // NOLINT (unsafe cast)
        );
        device.unmapMemory(*memory, dld);
    }

    return {
        std::move(buffer),
        std::move(memory)
    };
}

/* Memory allocations */

vk::UniqueDeviceMemory vkhelper::allocateMemory(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::PhysicalDevice& physdev,
    vk::DeviceSize size,
    std::bitset<32> types,
    bool hostVisible
) {
    // Find a suitable memory type index
    const auto memProps{physdev.getMemoryProperties2(dld)};

    std::optional<uint32_t> selectedTypeIdx{};
    for (uint32_t i = 0; i < memProps.memoryProperties.memoryTypeCount; i++) {
        if (!types.test(i))
            continue;
        const auto& memType{memProps.memoryProperties.memoryTypes.at(i)};

        const bool isHostVisible{
            memType.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible &&
            memType.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent
        };
        if (hostVisible && !isHostVisible)
            continue;

        selectedTypeIdx = i;

        if (memType.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
            break;

        // Fallback to host-visible memory if no device-local memory is available
    }

    if (!selectedTypeIdx)
        throw std::runtime_error("No suitable memory type found for allocation");

    // Allocate memory
    const vk::MemoryAllocateInfo allocInfo{
        .allocationSize = size,
        .memoryTypeIndex = *selectedTypeIdx
    };
    return device.allocateMemoryUnique(allocInfo, nullptr, dld);
}

/* Descriptors */

std::pair<vk::UniqueDescriptorPool, vk::DescriptorSet> vkhelper::createDescriptorSet(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::DescriptorSetLayout& layout,
    uint32_t samplers, uint32_t buffers,
    uint32_t sampledImages, uint32_t storageImages
) {
    const std::array<vk::DescriptorPoolSize, 4> poolSizes{{
        { .type = vk::DescriptorType::eSampler,
          .descriptorCount = samplers },
        { .type = vk::DescriptorType::eSampledImage,
          .descriptorCount = sampledImages },
        { .type = vk::DescriptorType::eStorageImage,
          .descriptorCount = storageImages },
        { .type = vk::DescriptorType::eUniformBuffer,
          .descriptorCount = buffers }
    }};
    auto pool{device.createDescriptorPoolUnique({
        .flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    }, nullptr, dld)};

    auto set{device.allocateDescriptorSets({
        .descriptorPool = *pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    }, dld).at(0)};

    return{
        std::move(pool),
        set
    };
}

vk::UniqueImageView vkhelper::createImageView(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::Image& image,
    vk::Format format,
    uint32_t layers
) {
    const vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = layers == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray,
        .format = format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = layers
        }
    };
    return device.createImageViewUnique(viewInfo, nullptr, dld);
}

/* Command buffers */

vk::UniqueCommandPool vkhelper::createCommandPool(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    uint32_t qfi
) {
    const vk::CommandPoolCreateInfo cmdpoolInfo{
        .queueFamilyIndex = qfi
    };
    return device.createCommandPoolUnique(cmdpoolInfo, nullptr, dld);
}

vk::UniqueCommandBuffer vkhelper::createCommandBuffer(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::CommandPool& cmdpool
) {
    const vk::CommandBufferAllocateInfo cmdbufInfo{
        .commandPool = cmdpool,
        .commandBufferCount = 1
    };
    return { std::move(device.allocateCommandBuffersUnique(cmdbufInfo, dld).front()) };
}

vk::UniqueSemaphore vkhelper::createTimelineSemaphore(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    bool exportable
) {
    const vk::ExportSemaphoreCreateInfo exportInfo{
        .handleTypes = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd
    };
    const vk::SemaphoreTypeCreateInfo typeInfo{
        .pNext = exportable ? &exportInfo : nullptr,
        .semaphoreType = vk::SemaphoreType::eTimeline,
    };
    const vk::SemaphoreCreateInfo createInfo{
        .pNext = &typeInfo,
    };
    return device.createSemaphoreUnique(createInfo, nullptr, dld);
}

vk::UniqueFence vkhelper::createFence(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device
) {
    return device.createFenceUnique({}, nullptr, dld);
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

int vkhelper::exportMemoryFd(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::DeviceMemory& memory
) {
    const vk::MemoryGetFdInfoKHR fdInfo{
        .memory = memory,
        .handleType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd
    };
    return device.getMemoryFdKHR(fdInfo, dld);
}

int vkhelper::exportSemaphoreFd(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    const vk::Semaphore& semaphore
) {
    const vk::SemaphoreGetFdInfoKHR fdInfo{
        .semaphore = semaphore,
        .handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd
    };
    return device.getSemaphoreFdKHR(fdInfo, dld);
}
