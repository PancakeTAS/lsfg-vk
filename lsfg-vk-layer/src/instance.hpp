/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "lsfg-vk/lsfgvk.hpp"
#include "lsfg-vk-common/configuration/config.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"
#include "swapchain.hpp"

#include <functional>
#include <optional>
#include <unordered_map>

#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    ///
    /// Root context of the lsfg-vk layer
    ///
    class Root {
    public:
        ///
        /// Create the lsfg-vk root context
        ///
        /// @throws ls::error on failure
        ///
        Root();

        ///
        /// Check if the layer is active
        ///
        /// @return true If active
        ///
        [[nodiscard]] bool active() const { return this->active_profile.has_value(); }

        ///
        /// Ensure the layer is up-to-date
        ///
        /// @return true If the configuration was updated
        ///
        bool update();

        ///
        /// Modify instance create info
        ///
        /// @param createInfo Original create info
        /// @param finish Function to call after modification
        ///
        void modifyInstanceCreateInfo(VkInstanceCreateInfo& createInfo,
            const std::function<void(void)>& finish) const;
        ///
        /// Modify device create info
        ///
        /// @param createInfo Original create info
        /// @param finish Function to call after modification
        ///
        void modifyDeviceCreateInfo(VkDeviceCreateInfo& createInfo,
            const std::function<void(void)>& finish) const;
        ///
        /// Modify swapchain create info
        ///
        /// @param vk Vulkan instance
        /// @param createInfo Original create info
        /// @param finish Function to call after modification
        ///
        void modifySwapchainCreateInfo(const vk::Vulkan& vk, VkSwapchainCreateInfoKHR& createInfo,
            const std::function<void(void)>& finish) const;

        ///
        /// Create swapchain context
        ///
        /// @param vk Vulkan instance
        /// @param swapchain Swapchain handle
        /// @param info Swapchain info
        /// @throws ls::error on failure
        ///
        void createSwapchainContext(const vk::Vulkan& vk, VkSwapchainKHR swapchain,
            const SwapchainInfo& info);
        ///
        /// Get swapchain context
        ///
        /// @param swapchain Swapchain handle
        /// @return swapchain Context
        /// @throws ls::error if not found
        ///
        [[nodiscard]] Swapchain& getSwapchainContext(VkSwapchainKHR swapchain) {
            const auto& it = this->swapchains.find(swapchain);
            if (it == this->swapchains.end())
                throw ls::error("swapchain context not found");

            return it->second;
        }
        ///
        /// Remove swapchain context
        ///
        /// @param swapchain Swapchain handle
        ///
        void removeSwapchainContext(VkSwapchainKHR swapchain);
    private:
        ls::WatchedConfig config;
        std::optional<ls::GameConf> active_profile;

        ls::lazy<lsfgvk::Instance> backend;
        std::unordered_map<VkSwapchainKHR, Swapchain> swapchains;
    };

}
