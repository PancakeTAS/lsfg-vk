#pragma once

#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace LSFG::Core {

    ///
    /// C++ wrapper class for a Vulkan descriptor pool.
    ///
    /// This class manages the lifetime of a Vulkan descriptor pool.
    ///
    class DescriptorPool {
    public:
        DescriptorPool() noexcept = default;

        ///
        /// Create the descriptor pool.
        ///
        /// @param device Vulkan device
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        DescriptorPool(const Core::Device& device);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->descriptorPool; }

        /// Trivially copyable, moveable and destructible
        DescriptorPool(const DescriptorPool&) noexcept = default;
        DescriptorPool& operator=(const DescriptorPool&) noexcept = default;
        DescriptorPool(DescriptorPool&&) noexcept = default;
        DescriptorPool& operator=(DescriptorPool&&) noexcept = default;
        ~DescriptorPool() = default;
    private:
        std::shared_ptr<VkDescriptorPool> descriptorPool;
    };

}
