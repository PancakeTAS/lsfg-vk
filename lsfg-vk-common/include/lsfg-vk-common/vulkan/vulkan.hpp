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

    /// vulkan instance function pointers
    struct VulkanInstanceFuncs {
        PFN_vkDestroyInstance DestroyInstance;
        PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
        PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;
        PFN_vkGetPhysicalDeviceProperties2 GetPhysicalDeviceProperties2;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties;
        PFN_vkGetPhysicalDeviceFeatures2 GetPhysicalDeviceFeatures2;
        PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
        PFN_vkCreateDevice CreateDevice;
        PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    };

    using PhysicalDeviceSelector = const std::function<
        VkPhysicalDevice(
            const VulkanInstanceFuncs&,
            const std::vector<VkPhysicalDevice>&
        )
    >&;

    /// vulkan device function pointers
    struct VulkanDeviceFuncs {
        PFN_vkGetDeviceQueue GetDeviceQueue;
        PFN_vkDeviceWaitIdle DeviceWaitIdle;
        PFN_vkCreateCommandPool CreateCommandPool;
        PFN_vkDestroyCommandPool DestroyCommandPool;
        PFN_vkCreateDescriptorPool CreateDescriptorPool;
        PFN_vkDestroyDescriptorPool DestroyDescriptorPool;
        PFN_vkCreateBuffer CreateBuffer;
        PFN_vkDestroyBuffer DestroyBuffer;
        PFN_vkGetBufferMemoryRequirements GetBufferMemoryRequirements;
        PFN_vkAllocateMemory AllocateMemory;
        PFN_vkFreeMemory FreeMemory;
        PFN_vkBindBufferMemory BindBufferMemory;
        PFN_vkMapMemory MapMemory;
        PFN_vkUnmapMemory UnmapMemory;
        PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
        PFN_vkFreeCommandBuffers FreeCommandBuffers;
        PFN_vkBeginCommandBuffer BeginCommandBuffer;
        PFN_vkEndCommandBuffer EndCommandBuffer;
        PFN_vkCmdPipelineBarrier CmdPipelineBarrier;
        PFN_vkCmdClearColorImage CmdClearColorImage;
        PFN_vkCmdBindPipeline CmdBindPipeline;
        PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets;
        PFN_vkCmdDispatch CmdDispatch;
        PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage;
        PFN_vkQueueSubmit QueueSubmit;
        PFN_vkAllocateDescriptorSets AllocateDescriptorSets;
        PFN_vkFreeDescriptorSets FreeDescriptorSets;
        PFN_vkUpdateDescriptorSets UpdateDescriptorSets;
        PFN_vkCreateFence CreateFence;
        PFN_vkDestroyFence DestroyFence;
        PFN_vkResetFences ResetFences;
        PFN_vkWaitForFences WaitForFences;
        PFN_vkCreateImage CreateImage;
        PFN_vkDestroyImage DestroyImage;
        PFN_vkGetImageMemoryRequirements GetImageMemoryRequirements;
        PFN_vkBindImageMemory BindImageMemory;
        PFN_vkCreateImageView CreateImageView;
        PFN_vkDestroyImageView DestroyImageView;
        PFN_vkCreateSampler CreateSampler;
        PFN_vkDestroySampler DestroySampler;
        PFN_vkCreateSemaphore CreateSemaphore;
        PFN_vkDestroySemaphore DestroySemaphore;
        PFN_vkCreateShaderModule CreateShaderModule;
        PFN_vkDestroyShaderModule DestroyShaderModule;
        PFN_vkCreateDescriptorSetLayout CreateDescriptorSetLayout;
        PFN_vkDestroyDescriptorSetLayout DestroyDescriptorSetLayout;
        PFN_vkCreatePipelineLayout CreatePipelineLayout;
        PFN_vkDestroyPipelineLayout DestroyPipelineLayout;
        PFN_vkCreateComputePipelines CreateComputePipelines;
        PFN_vkDestroyPipeline DestroyPipeline;
        PFN_vkSignalSemaphore SignalSemaphore;
        PFN_vkWaitSemaphores WaitSemaphores;

        // extension functions
        PFN_vkGetMemoryFdKHR GetMemoryFdKHR;
        PFN_vkImportSemaphoreFdKHR ImportSemaphoreFdKHR;
        PFN_vkGetSemaphoreFdKHR GetSemaphoreFdKHR;
    };

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

        /// get the vulkan instance
        /// @return the instance handle
        [[nodiscard]] const auto& inst() const { return this->instance.get(); }
        /// get the vulkan device
        /// @return the device handle
        [[nodiscard]] const auto& dev() const { return this->device.get(); }
        /// get the command pool
        /// @return the command pool handle
        [[nodiscard]] const auto& cmdpool() const { return this->cmdPool.get(); }
        /// get the descriptor pool
        /// @return the descriptor pool handle
        [[nodiscard]] const auto& descpool() const { return this->descPool.get(); }
        /// get the compute queue
        /// @return the compute queue handle
        [[nodiscard]] const auto& queue() const { return this->computeQueue; }

        /// check if fp16 is supported
        /// @return true if fp16 is supported
        [[nodiscard]] bool supportsFP16() const { return this->fp16; }

        /// get device-level function pointers
        /// @return the device function pointers
        [[nodiscard]] const auto& df() const { return this->device_funcs; }
    private:
        ls::owned_ptr<VkInstance> instance;
        VulkanInstanceFuncs instance_funcs;

        VkPhysicalDevice physdev;
        uint32_t computeFamilyIdx;
        bool fp16;

        ls::owned_ptr<VkDevice> device;
        VulkanDeviceFuncs device_funcs;

        VkQueue computeQueue;

        ls::owned_ptr<VkCommandPool> cmdPool;
        ls::owned_ptr<VkDescriptorPool> descPool;
    };
}
