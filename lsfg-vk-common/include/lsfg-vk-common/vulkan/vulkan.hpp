#pragma once

#include "../helpers/pointers.hpp"

#include <bitset>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vk {
    using PhysicalDeviceSelector = const std::function<
        VkPhysicalDevice(const std::vector<VkPhysicalDevice>&)
    >&;

    /// vulkan version wrapper
    class version {
    public:
        /// construct from version numbers
        version(uint8_t major, uint8_t minor, uint8_t patch)
            : major(major), minor(minor), patch(patch) {}

        /// convert to Vulkan version
        [[nodiscard]] uint32_t into() const {
            return VK_MAKE_VERSION(major, minor, patch);
        }
    private:
        uint8_t major{};
        uint8_t minor{};
        uint8_t patch{};
    };

    /// vulkan instance
    class Vulkan {
    public:
        /// create a vulkan instance
        /// @param appName name of the application
        /// @param appVersion version of the application
        /// @param engineName name of the engine
        /// @param engineVersion version of the engine
        /// @param selectPhysicalDevice function to select the physical device
        /// @throws ls::vulkan_error on failure
        Vulkan(const std::string& appName, version appVersion,
            const std::string& engineName, version engineVersion,
            PhysicalDeviceSelector selectPhysicalDevice);

        /// find a memory type index
        /// @param validTypes bitset of valid memory types
        /// @param hostVisibility whether the memory should be host visible
        /// @return the memory type index
        [[nodiscard]] std::optional<uint32_t> findMemoryTypeIndex(
            std::bitset<32> validTypes, bool hostVisibility) const;

        /// get the vulkan device
        /// @return the device handle
        [[nodiscard]] const auto& dev() const { return this->device.get(); }
        /// get the command pool
        /// @return the command pool handle
        [[nodiscard]] const auto& cmdpool() const { return this->cmdPool.get(); }
        /// get the descriptor pool
        /// @return the descriptor pool handle
        [[nodiscard]] const auto& descpool() const { return this->descPool.get(); }

        /// check if fp16 is supported
        /// @return true if fp16 is supported
        [[nodiscard]] bool supportsFP16() const { return this->fp16; }
    private:
        ls::owned_ptr<VkInstance> instance;

        VkPhysicalDevice physdev;
        uint32_t computeFamilyIdx;
        bool fp16;

        ls::owned_ptr<VkDevice> device;
        VkQueue computeQueue;

        ls::owned_ptr<VkCommandPool> cmdPool;
        ls::owned_ptr<VkDescriptorPool> descPool;
    };
}
