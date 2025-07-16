#pragma once

#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace LSFG::Core {

    ///
    /// C++ wrapper class for a Vulkan command pool.
    ///
    /// This class manages the lifetime of a Vulkan command pool.
    ///
    class CommandPool {
    public:
        CommandPool() noexcept = default;

        ///
        /// Create the command pool.
        ///
        /// @param device Vulkan device
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        CommandPool(const Core::Device& device);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->commandPool; }

        /// Trivially copyable, moveable and destructible
        CommandPool(const CommandPool&) noexcept = default;
        CommandPool& operator=(const CommandPool&) noexcept = default;
        CommandPool(CommandPool&&) noexcept = default;
        CommandPool& operator=(CommandPool&&) noexcept = default;
        ~CommandPool() = default;
    private:
        std::shared_ptr<VkCommandPool> commandPool;
    };

}
