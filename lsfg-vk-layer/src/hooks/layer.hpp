/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/configuration/config.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    /// device wrapper class
    class MyVkLayer {
    public:
        /// create a vulkan device wrapper
        /// @throws ls::vulkan_error on failure
        MyVkLayer();

        /// ensure the layer is up-to-date
        /// @return true if the configuration was updated
        bool update();

        /// check if there is an active profile
        /// @return true if there is an active profile
        [[nodiscard]] bool isActive() const { return this->current_profile.has_value(); }

        /// get the global configuration
        /// @return configuration reference
        [[nodiscard]] const auto& global() const { return this->config.get().global(); }
        /// get the active profile, if any
        /// @return profile optional
        [[nodiscard]] const auto& profile() const { return *this->current_profile; }

        /// get or create the backend instance
        /// @return backend instance reference
        /// @throws ls::error if an error occured during backend creation
        [[nodiscard]] backend::Instance& backend();

        // non-moveable, non-copyable
        MyVkLayer(const MyVkLayer&) = delete;
        MyVkLayer& operator=(const MyVkLayer&) = delete;
        MyVkLayer(MyVkLayer&&) = delete;
        MyVkLayer& operator=(MyVkLayer&&) = delete;
        ~MyVkLayer() = default;
    private:
        ls::WatchedConfig config;
        std::optional<ls::GameConf> current_profile;

        ls::lazy<backend::Instance> backend_instance;
    };

}
