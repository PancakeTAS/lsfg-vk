#include "instance.hpp"
#include "swapchain.hpp"
#include "lsfg-vk-common/configuration/detection.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk;
using namespace lsfgvk::layer;

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
    // find the shader dll
    std::filesystem::path findShaderDll() {
        const std::vector<std::filesystem::path> FRAGMENTS{{
            ".local/share/Steam/steamapps/common",
            ".steam/steam/steamapps/common",
            ".steam/debian-installation/steamapps/common",
            ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common",
            "snap/steam/common/.local/share/Steam/steamapps/common"
        }};

        // check XDG overridden location
        const char* xdgPath = std::getenv("XDG_DATA_HOME");
        if (xdgPath && *xdgPath != '\0') {
            auto base = std::filesystem::path(xdgPath);

            for (const auto& frag : FRAGMENTS) {
                auto full = base / frag / "Lossless Scaling" / "Lossless.dll";
                if (std::filesystem::exists(full))
                    return full;
            }
        }

        // check home directory
        const char* homePath = std::getenv("HOME");
        if (homePath && *homePath != '\0') {
            auto base = std::filesystem::path(homePath);

            for (const auto& frag : FRAGMENTS) {
                auto full = base / frag / "Lossless Scaling" / "Lossless.dll";
                if (std::filesystem::exists(full))
                    return full;
            }
        }

        // fallback to same directory
        auto local = std::filesystem::current_path() / "Lossless.dll";
        if (std::filesystem::exists(local))
            return local;

        throw ls::error("unable to locate Lossless.dll, please set the path in the configuration");
    }
}

Root::Root() {
    // find active profile
    this->config.reload();
    const auto& profile = findProfile(this->config, ls::identify());
    if (!profile.has_value())
        return;

    this->active_profile = profile->second;

    std::cerr << "lsfg-vk: using profile with name '" << this->active_profile->name << "' ";
    switch (profile->first) {
        case ls::IdentType::OVERRIDE:
            std::cerr << "(identified via override)\n";
            break;
        case ls::IdentType::EXECUTABLE:
            std::cerr << "(identified via executable)\n";
            break;
        case ls::IdentType::WINE_EXECUTABLE:
            std::cerr << "(identified via wine executable)\n";
            break;
        case ls::IdentType::PROCESS_NAME:
            std::cerr << "(identified via process name)\n";
            break;
    }
}

void Root::update() {
    if (!this->config.isUpToDate())
        this->config.reload();
}

void Root::modifyInstanceCreateInfo(VkInstanceCreateInfo& createInfo,
        const std::function<void(void)>& finish) const {
    if (!this->active_profile.has_value())
        return;

    auto extensions = add_extensions(
        createInfo.ppEnabledExtensionNames,
        createInfo.enabledExtensionCount,
        {
            "VK_KHR_get_physical_device_properties2",
            "VK_KHR_external_memory_capabilities",
            "VK_KHR_external_semaphore_capabilities"
        }
    );
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    finish();
}

void Root::modifyDeviceCreateInfo(VkDeviceCreateInfo& createInfo,
        const std::function<void(void)>& finish) const {
    if (!this->active_profile.has_value())
        return;

    auto extensions = add_extensions(
        createInfo.ppEnabledExtensionNames,
        createInfo.enabledExtensionCount,
        {
            "VK_KHR_external_memory",
            "VK_KHR_external_memory_fd",
            "VK_KHR_external_semaphore",
            "VK_KHR_external_semaphore_fd",
            "VK_KHR_timeline_semaphore"
        }
    );
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    bool isFeatureEnabled = false;
    auto* featureInfo = reinterpret_cast<VkBaseInStructure*>(const_cast<void*>(createInfo.pNext));
    while (featureInfo) {
        if (featureInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES) {
            auto* features = reinterpret_cast<VkPhysicalDeviceVulkan12Features*>(featureInfo);
            features->timelineSemaphore = VK_TRUE;
            isFeatureEnabled = true;
        } else if (featureInfo->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES) {
            auto* features = reinterpret_cast<VkPhysicalDeviceTimelineSemaphoreFeatures*>(featureInfo);
            features->timelineSemaphore = VK_TRUE;
            isFeatureEnabled = true;
        }

        featureInfo = const_cast<VkBaseInStructure*>(featureInfo->pNext);
    }

    VkPhysicalDeviceTimelineSemaphoreFeatures timelineFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
        .pNext = const_cast<void*>(createInfo.pNext),
        .timelineSemaphore = VK_TRUE
    };
    if (!isFeatureEnabled)
        createInfo.pNext = &timelineFeatures;

    finish();
}

void Root::modifySwapchainCreateInfo(const vk::Vulkan& vk, VkSwapchainCreateInfoKHR& createInfo,
        const std::function<void(void)>& finish) const {
    if (!this->active_profile.has_value())
        return;

    VkSurfaceCapabilitiesKHR caps{};
    auto res = vk.fi().GetPhysicalDeviceSurfaceCapabilitiesKHR(
        vk.physdev(), createInfo.surface, &caps);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed");

    context_ModifySwapchainCreateInfo(*this->active_profile, caps.maxImageCount, createInfo);

    finish();
}

void Root::createSwapchainContext(const vk::Vulkan& vk,
        VkSwapchainKHR swapchain, const SwapchainInfo& info) {
    if (!this->active_profile.has_value())
        throw ls::error("attempted to create swapchain context while layer is inactive");
    const auto& profile = *this->active_profile;

    if (!this->backend.has_value()) { // emplace backend late, due to loader bug
        const auto& global = this->config.getGlobalConf();

        setenv("DISABLE_LSFGVK", "1", 1); // NOLINT (c++-include)

        try {
            this->backend.emplace(
                [gpu = profile.gpu](
                    const std::string& deviceName,
                    std::pair<const std::string&, const std::string&> ids,
                    const std::optional<std::string>& pci
                ) {
                    if (!gpu)
                        return true;

                    return (deviceName == *gpu)
                        || (ids.first + ":" + ids.second == *gpu)
                        || (pci && *pci == *gpu);
                },
                global.dll.value_or(findShaderDll()),
                global.allow_fp16
            );
        } catch (const std::exception& e) {
            unsetenv("DISABLE_LSFGVK"); // NOLINT (c++-include)
            throw ls::error("failed to create backend instance", e);
        }

        unsetenv("DISABLE_LSFGVK"); // NOLINT (c++-include)
    }

    this->swapchains.emplace(swapchain,
        Swapchain(vk, this->backend.mut(), profile, info));
}

void Root::removeSwapchainContext(VkSwapchainKHR swapchain) {
    this->swapchains.erase(swapchain);
}
