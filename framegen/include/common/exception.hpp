#pragma once

#include <vulkan/vulkan_core.h>

#include <exception>
#include <stdexcept>
#include <string>

namespace LSFG {

    /// Simple exception class for Vulkan errors.
    class vulkan_error : public std::runtime_error {
    public:
        ///
        /// Construct a vulkan_error with a message and a Vulkan result code.
        ///
        /// @param result The Vulkan result code associated with the error.
        /// @param message The error message.
        ///
        explicit vulkan_error(VkResult result, const std::string& message);

        /// Get the Vulkan result code associated with this error.
        [[nodiscard]] VkResult error() const { return this->result; }

        // Trivially copyable, moveable and destructible
        vulkan_error(const vulkan_error&) = default;
        vulkan_error(vulkan_error&&) = default;
        vulkan_error& operator=(const vulkan_error&) = default;
        vulkan_error& operator=(vulkan_error&&) = default;
        ~vulkan_error() noexcept override;
    private:
        VkResult result;
    };

    /// Simple exception class for stacking errors.
    class rethrowable_error : public std::runtime_error {
    public:
        ///
        /// Construct a new rethrowable_error with a message.
        ///
        /// @param message The error message.
        /// @param exe The original exception to rethrow.
        ///
        explicit rethrowable_error(const std::string& message,
            const std::exception& exe);

        /// Get the exception as a string.
        [[nodiscard]] const char* what() const noexcept override {
            return message.c_str();
        }

        // Trivially copyable, moveable and destructible
        rethrowable_error(const rethrowable_error&) = default;
        rethrowable_error(rethrowable_error&&) = default;
        rethrowable_error& operator=(const rethrowable_error&) = default;
        rethrowable_error& operator=(rethrowable_error&&) = default;
        ~rethrowable_error() noexcept override;
    private:
        std::string message;
    };

}
