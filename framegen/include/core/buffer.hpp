#pragma once

#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <memory>

namespace LSFG::Core {

    ///
    /// C++ wrapper class for a Vulkan buffer.
    ///
    /// This class manages the lifetime of a Vulkan buffer.
    ///
    class Buffer {
    public:
        Buffer() noexcept = default;

        ///
        /// Create the buffer.
        ///
        /// @param device Vulkan device
        /// @param data Initial data for the buffer, also specifies the size of the buffer.
        /// @param usage Usage flags for the buffer
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        template<typename T>
        Buffer(const Core::Device& device, const T& data, VkBufferUsageFlags usage)
                : size(sizeof(T)) {
            construct(device, reinterpret_cast<const void*>(&data), usage);
        }

        ///
        /// Create the buffer.
        ///
        /// @param device Vulkan device
        /// @param data Initial data for the buffer
        /// @param size Size of the buffer in bytes
        /// @param usage Usage flags for the buffer
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        Buffer(const Core::Device& device, const void* data, size_t size, VkBufferUsageFlags usage)
                : size(size) {
            construct(device, data, usage);
        }

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->buffer; }
        /// Get the size of the buffer.
        [[nodiscard]] size_t getSize() const { return this->size; }

        /// Trivially copyable, moveable and destructible
        Buffer(const Buffer&) noexcept = default;
        Buffer& operator=(const Buffer&) noexcept = default;
        Buffer(Buffer&&) noexcept = default;
        Buffer& operator=(Buffer&&) noexcept = default;
        ~Buffer() = default;
    private:
        void construct(const Core::Device& device, const void* data, VkBufferUsageFlags usage);

        std::shared_ptr<VkBuffer> buffer;
        std::shared_ptr<VkDeviceMemory> memory;

        size_t size{};
    };

}
