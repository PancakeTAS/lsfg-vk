/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "layer.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <functional>

#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    /// instance wrapper class
    class MyVkInstance {
    public:
        /// create a vulkan instance wrapper
        /// @param layer layer reference
        /// @param info base instance info
        /// @param addr function to get instance proc addresses
        /// @param createFunc function to create the instance
        /// @throws ls::vulkan_error on vulkan errors
        MyVkInstance(MyVkLayer& layer,
            VkInstanceCreateInfo info, PFN_vkGetInstanceProcAddr addr,
            const std::function<VkInstance(VkInstanceCreateInfo*)>& createFunc);

        /// get the referenced vulkan instance
        /// @return vulkan instance
        [[nodiscard]] const auto& instance() const { return handle; }
        /// get the vulkan instance functions
        /// @return vulkan instance functions
        [[nodiscard]] const auto& funcs() const { return ifuncs; }

        // non-moveable, non-copyable
        MyVkInstance(const MyVkInstance&) = delete;
        MyVkInstance& operator=(const MyVkInstance&) = delete;
        MyVkInstance(MyVkInstance&&) = delete;
        MyVkInstance& operator=(MyVkInstance&&) = delete;
        ~MyVkInstance() = default;
    private:
        ls::R<MyVkLayer> layer;

        VkInstance handle;
        vk::VulkanInstanceFuncs ifuncs{};
    };

}
