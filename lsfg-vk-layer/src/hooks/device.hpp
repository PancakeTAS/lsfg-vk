/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "instance.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <functional>

#include <mutex>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

namespace lsfgvk::layer {

    /// simple struct for multithreaded queue access
    struct OffloadQueue {
        VkQueue queue;
        std::mutex mutex;
    };

    /// device wrapper class
    class MyVkDevice {
    public:
        /// create a vulkan device wrapper
        /// @param layer layer reference
        /// @param instance parent vulkan instance
        /// @param info base device info
        /// @param physdev physical device to create the device for
        /// @param addr function to get device proc addresses
        /// @param loader_addr function to set loader data
        /// @param createFunc function to create the device
        /// @throws ls::vulkan_error on vulkan errors
        MyVkDevice(MyVkLayer& layer, MyVkInstance& instance,
            VkPhysicalDevice physdev, VkDeviceCreateInfo info,
            PFN_vkGetDeviceProcAddr addr, PFN_vkSetDeviceLoaderData loader_addr,
            const std::function<VkDevice(VkDeviceCreateInfo*)>& createFunc);

        /// get the referenced vulkan device
        /// @return vulkan device
        [[nodiscard]] const auto& device() const { return handle; }
        /// get the vulkan device functions
        /// @return vulkan device functions
        [[nodiscard]] const auto& funcs() const { return dfuncs; }

        /// get the vulkan instance
        /// @return vulkan instance
        [[nodiscard]] const auto& vkd() const { return *vk; }
        /// get the additional queue
        /// @return vulkan queue
        [[nodiscard]] auto& offload() { return offloadQueue; }

        // non-moveable, non-copyable
        MyVkDevice(const MyVkDevice&) = delete;
        MyVkDevice& operator=(const MyVkDevice&) = delete;
        MyVkDevice(MyVkDevice&&) = delete;
        MyVkDevice& operator=(MyVkDevice&&) = delete;
        ~MyVkDevice() = default;
    private:
        ls::R<MyVkLayer> layer;
        ls::R<MyVkInstance> instance;

        VkDevice handle;
        vk::VulkanDeviceFuncs dfuncs{};

        ls::lazy<vk::Vulkan> vk;
        OffloadQueue offloadQueue{};
    };

}
