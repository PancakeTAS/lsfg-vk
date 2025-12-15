#pragma once

#include "config.hpp"
#include "detection.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <optional>
#include <vector>

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
        [[nodiscard]] std::vector<const char*> instanceExtensions() const;
        /// required device extensions
        /// @return list of extension names
        [[nodiscard]] std::vector<const char*> deviceExtensions() const;

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
        /// @param vk vulkan instance
        LayerInstance(const Layer& layer, vk::Vulkan vk);
    private:
        vk::Vulkan vk;
    };

}
