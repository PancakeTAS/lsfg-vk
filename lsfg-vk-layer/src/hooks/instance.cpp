/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "instance.hpp"
#include "layer.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
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
}

MyVkInstance::MyVkInstance(MyVkLayer& layer,
            VkInstanceCreateInfo info, PFN_vkGetInstanceProcAddr addr,
            const std::function<VkInstance(VkInstanceCreateInfo*)>& createFunc) :
        layer(std::ref(layer)) {
    auto extensions = add_extensions(
        info.ppEnabledExtensionNames,
        info.enabledExtensionCount,
        {
            "VK_KHR_get_physical_device_properties2",
            "VK_KHR_external_memory_capabilities",
            "VK_KHR_external_semaphore_capabilities"
        }
    );
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();

    // create instance
    this->handle = createFunc(&info); // NOLINT (prefer member initializer)
    this->ifuncs = vk::initVulkanInstanceFuncs(this->handle, addr, true);
}
