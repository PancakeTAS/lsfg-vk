/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_DEFAULT_DISPATCHER 1
#define VULKAN_HPP_NO_CONSTRUCTORS 1
#include <vulkan/vulkan.hpp> // IWYU pragma: export

// IWYU pragma: begin_exports
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_funcs.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_hpp_macros.hpp>
#include <vulkan/vulkan_structs.hpp>
// IWYU pragma: end_exports

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace vkhelper {

    /* Device initialization */

    ///
    /// Create a Vulkan 1.2 instance for lsfg-vk
    ///
    /// @param dld Dynamic dispatch loader
    /// @return RAII-wrapped Vulkan instance
    /// @throws std::runtime_error on failure
    ///
    vk::UniqueInstance createInstance(vk::detail::DispatchLoaderDynamic& dld);

    ///
    /// Find a physical device through a custom identifier
    ///
    /// The custom identifier may be one of:
    /// - Device name (e.g. "NVIDIA GeForce RTX 5080")
    /// - Vendor ID + Device ID in lowercase hexadecimal (e.g. "10de:2c02")
    /// - PCI bus ID with padded zeroes (e.g. "0000:01:00.0")
    ///
    /// @param dld Dynamic dispatch loader
    /// @param instance Vulkan instance
    /// @param id Custom identifier
    /// @return Selected physical device
    /// @throws std::runtime_error if no suitable device found
    ///
    vk::PhysicalDevice findPhysicalDevice(
        const vk::detail::DispatchLoaderDynamic& dld,
        const vk::Instance& instance,
        const std::string& id
    );

    ///
    /// Find the first compute-capable queue family index
    ///
    /// @param dld Dynamic dispatch loader
    /// @param physdev Physical device
    /// @return Queue family index
    /// @throws std::runtime_error if no compute-capable queue found
    ///
    uint32_t findComputeQueueFamilyIndex(
        const vk::detail::DispatchLoaderDynamic& dld,
        const vk::PhysicalDevice& physdev
    );

    ///
    /// Check a physical device for half-precision float support
    ///
    /// @param dld Dynamic dispatch loader
    /// @param physdev Physical device
    /// @return Whether half-precision float is supported
    ///
    bool checkHalfPrecisionSupport(
        const vk::detail::DispatchLoaderDynamic& dld,
        const vk::PhysicalDevice& physdev
    );

    ///
    /// Create a Vulkan device for lsfg-vk
    ///
    /// This device will have the core features timelineSemaphore and shaderFloat16 (if requested)
    /// enabled, as well as the synchronization2, external memory & semaphore fd extensions.
    ///
    /// @param dld Dynamic dispatch loader
    /// @param physdev Physical device
    /// @param qfi Queue family index of compute-capable queue
    /// @param fp16 Whether to enable half-precision float support
    /// @return RAII-wrapped Vulkan device & compute queue
    /// @throws std::runtime_error on failure
    ///
    std::pair<vk::UniqueDevice, vk::Queue> createDevice(
        vk::detail::DispatchLoaderDynamic& dld,
        const vk::PhysicalDevice& physdev,
        uint32_t qfi,
        bool fp16
    );

    /* Shader modules & pipelines */

    ///
    /// Create a Vulkan shader module from SPIR-V bytecode
    ///
    /// @param dld Dynamic dispatch loader
    /// @param device Vulkan device
    /// @param code SPIR-V bytecode
    /// @return RAII-wrapped Vulkan shader module
    /// @throws std::runtime_error on failure
    ///
    vk::UniqueShaderModule createShaderModule(
        const vk::detail::DispatchLoaderDynamic& dld,
        const vk::Device& device,
        const std::span<const uint32_t>& code
    );

    ///
    /// Create a Vulkan descriptor set layout
    ///
    /// @param dld Dynamic dispatch loader
    /// @param device Vulkan device
    /// @param bindings List of descriptor set layout bindings
    /// @param pushConstantSize Size of push constant range
    /// @return RAII-wrapped Vulkan descriptor set & pipeline layout
    /// @throws std::runtime_error on failure
    ///
    std::pair<vk::UniqueDescriptorSetLayout, vk::UniquePipelineLayout> createLayout(
        const vk::detail::DispatchLoaderDynamic& dld,
        const vk::Device& device,
        const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
        size_t pushConstantSize
    );

    /* Resources */

    ///
    /// Create a (unallocated) Vulkan image for lsfg-vk
    ///
    /// @param dld Dynamic dispatch loader
    /// @param device Vulkan device
    /// @param extent Image extent
    /// @param format Image format
    /// @param layers Amount of images
    /// @param usage Image usage flags
    /// @return RAII-wrapped Vulkan image
    /// @throws std::runtime_error on failure
    ///
    vk::UniqueImage createImage(
        const vk::detail::DispatchLoaderDynamic& dld,
        const vk::Device& device,
        vk::Extent2D extent,
        vk::Format format,
        uint32_t layers,
        vk::ImageUsageFlags usage
    );

    ///
    /// Align a memory allocation
    ///
    /// @param size Memory size
    /// @param align Alignment
    /// @return Aligned memory size
    ///
    inline vk::DeviceSize align(vk::DeviceSize size, vk::DeviceSize align) noexcept {
        return (size + align - 1) & ~(align - 1);
    }

    /* External memory */

    ///
    /// Create a Vulkan image with a fd-exportable dedicated allocation
    ///
    /// @param dld Dynamic dispatch loader
    /// @param device Vulkan device
    /// @param physdev Physical device
    /// @param extent Image extent
    /// @param format Image format
    /// @param layers Amount of images
    /// @param usage Image usage flags
    /// @return RAII-wrapped Vulkan image
    /// @throws std::runtime_error on failure
    ///
    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> createExternalImage(
        const vk::detail::DispatchLoaderDynamic& dld,
        const vk::Device& device,
        const vk::PhysicalDevice& physdev,
        vk::Extent2D extent,
        vk::Format format,
        uint32_t layers,
        vk::ImageUsageFlags usage
    );

}
