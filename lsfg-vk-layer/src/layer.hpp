#pragma once

#include "config.hpp"
#include "detection.hpp"

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    /// lsfg-vk layer
    class Layer {
    public:
        /// create a new layer
        /// @throws lsfgvk::error on failure
        Layer();

        /// get the active profile
        /// @return game configuration
        [[nodiscard]] const auto& active() const { return profile; }

        /// required instance extensions
        /// @return list of extension names
        [[nodiscard]] std::vector<std::string> instanceExtensions() const;
        /// required device extensions
        /// @return list of extension names
        [[nodiscard]] std::vector<std::string> deviceExtensions() const;

        /// tick the layer
        /// @throws lsfgvk::error on failure
        /// @return true if profile changed
        bool tick();
    private:
        Configuration config;
        Identification identification;

        IdentType identType{}; // type used to deduce the profile
        std::optional<GameConf> profile;
    };


    /// instance of the lsfg-vk layer on a VkInstance/VkDevice pair.
    class LayerInstance {
    public:
        /// create a new layer instance
        /// @param layer parent layer
        /// @param instance Vulkan instance
        /// @param device Vulkan device
        /// @param setLoaderData function to set device loader data
        LayerInstance(const Layer& layer,
            VkInstance instance, VkDevice device,
            PFN_vkSetDeviceLoaderData setLoaderData);
    private:
        Configuration config;
    };

}
