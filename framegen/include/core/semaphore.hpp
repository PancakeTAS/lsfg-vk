#pragma once

#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <memory>

namespace LSFG::Core {

    ///
    /// C++ wrapper class for a Vulkan semaphore.
    ///
    /// This class manages the lifetime of a Vulkan semaphore.
    ///
    class Semaphore {
    public:
        Semaphore() noexcept = default;

        ///
        /// Create the semaphore.
        ///
        /// @param device Vulkan device
        /// @param initial Optional initial value for creating a timeline semaphore.
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        Semaphore(const Core::Device& device, std::optional<uint32_t> initial = std::nullopt);

        ///
        /// Import a semaphore.
        ///
        /// @param device Vulkan device
        /// @param fd File descriptor to import the semaphore from.
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        Semaphore(const Core::Device& device, int fd);

        ///
        /// Signal the semaphore to a specific value.
        ///
        /// @param device Vulkan device
        /// @param value The value to signal the semaphore to.
        ///
        /// @throws std::logic_error if the semaphore is not a timeline semaphore.
        /// @throws LSFG::vulkan_error if signaling fails.
        ///
        void signal(const Core::Device& device, uint64_t value) const;

        ///
        /// Wait for the semaphore to reach a specific value.
        ///
        /// @param device Vulkan device
        /// @param value The value to wait for.
        /// @param timeout The timeout in nanoseconds, or UINT64_MAX for no timeout.
        /// @returns true if the semaphore reached the value, false if it timed out.
        ///
        /// @throws std::logic_error if the semaphore is not a timeline semaphore.
        /// @throws LSFG::vulkan_error if waiting fails.
        ///
        [[nodiscard]] bool wait(const Core::Device& device, uint64_t value, uint64_t timeout = UINT64_MAX) const;

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->semaphore; }

        // Trivially copyable, moveable and destructible
        Semaphore(const Semaphore&) noexcept = default;
        Semaphore& operator=(const Semaphore&) noexcept = default;
        Semaphore(Semaphore&&) noexcept = default;
        Semaphore& operator=(Semaphore&&) noexcept = default;
        ~Semaphore() = default;
    private:
        std::shared_ptr<VkSemaphore> semaphore;
        bool isTimeline{};
    };

}
