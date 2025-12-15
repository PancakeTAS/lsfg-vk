#pragma once

#include "../configuration/config.hpp"
#include "../configuration/detection.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <optional>
#include <vector>

namespace lsfgvk::layer {

    /// root of the lsfg-vk layer
    class Root {
    public:
        /// create a new root
        /// @throws lsfgvk::error on failure
        Root();

        /// get the global configuration
        /// @return configuration
        [[nodiscard]] const auto& conf() const { return config.getGlobalConf(); }
        /// get the active profile
        /// @return game configuration
        [[nodiscard]] const auto& active() const { return profile; }

        /// required instance extensions
        /// @return list of extension names
        [[nodiscard]] std::vector<const char*> instanceExtensions() const;
        /// required device extensions
        /// @return list of extension names
        [[nodiscard]] std::vector<const char*> deviceExtensions() const;

        /// tick the root
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
    class Instance {
    public:
        /// create a new layer instance
        /// @param root root of the layer
        /// @param vk vulkan instance
        Instance(const Root& root, vk::Vulkan vk);
    private:
        vk::Vulkan vk;
        ls::lazy<lsfgvk::Instance> backend; // lazy due to KhronosGroup/Vulkan-Loader#1739
    };

}
