#pragma once

#include <string>
#include <vector>

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

namespace lsfgvk {

    /// required instance extensions
    /// @return list of extension names
    std::vector<std::string> requiredInstanceExtensions() noexcept;

    /// required device extensions
    /// @return list of extension names
    std::vector<std::string> requiredDeviceExtensions() noexcept;

    /// instance of the lsfg-vk layer on a VkInstance/VkDevice pair.
    class LayerInstance {
    public:
        /// create a new layer instance
        /// @param instance Vulkan instance
        /// @param device Vulkan device
        /// @param setLoaderData function to set device loader data
        LayerInstance(VkInstance instance, VkDevice device,
            PFN_vkSetDeviceLoaderData setLoaderData);
    };

}
