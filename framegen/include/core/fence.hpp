#pragma once

#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>

namespace LSFG::Core {

    ///
    /// C++ wrapper class for a Vulkan fence.
    ///
    /// This class manages the lifetime of a Vulkan fence.
    ///
    class Fence {
    public:
        Fence() noexcept = default;

        ///
        /// Create the fence.
        ///
        /// @param device Vulkan device
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        Fence(const Core::Device& device);

        ///
        /// Reset the fence to an unsignaled state.
        ///
        /// @param device Vulkan device
        ///
        /// @throws LSFG::vulkan_error if resetting fails.
        ///
        void reset(const Core::Device& device) const;

        ///
        /// Wait for the fence
        ///
        /// @param device Vulkan device
        /// @param timeout The timeout in nanoseconds, or UINT64_MAX for no timeout.
        /// @returns true if the fence signaled, false if it timed out.
        ///
        /// @throws LSFG::vulkan_error if waiting fails.
        ///
        [[nodiscard]] bool wait(const Core::Device& device, uint64_t timeout = UINT64_MAX) const;

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->fence; }

        // Trivially copyable, moveable and destructible
        Fence(const Fence&) noexcept = default;
        Fence& operator=(const Fence&) noexcept = default;
        Fence(Fence&&) noexcept = default;
        Fence& operator=(Fence&&) noexcept = default;
        ~Fence() = default;
    private:
        std::shared_ptr<VkFence> fence;
    };

}
