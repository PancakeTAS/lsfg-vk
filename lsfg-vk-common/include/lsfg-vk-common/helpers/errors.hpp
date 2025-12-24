/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <exception>
#include <stdexcept>
#include <string>

#include <vulkan/vulkan_core.h>

namespace ls {
    /// simple vulkan error type
    class vulkan_error : public std::runtime_error {
    public:
        /// construct a vulkan_error
        /// @param result the Vulkan result code
        /// @param msg the error message
        explicit vulkan_error(VkResult result, const std::string& msg)
            : std::runtime_error(msg + " (error " + std::to_string(result) + ")"),
              result(result) {}

        /// construct a vulkan_error
        /// @param msg the error message
        explicit vulkan_error(const std::string& msg)
            : vulkan_error(VK_ERROR_INITIALIZATION_FAILED, msg) {}

        /// get the Vulkan result code associated with this error
        [[nodiscard]] virtual VkResult error() const;
    private:
        VkResult result;
    };

    /// simple error type
    class [[gnu::visibility("default")]] error : public std::runtime_error {
    public:
        /// construct an error around an inner exception
        /// @param msg error message
        /// @param inner inner exception
        explicit error(const std::string& msg, const std::exception& inner)
            : std::runtime_error(msg + "\n- " + inner.what()), ex(inner) {}

        /// construct an error
        /// @param msg error message
        explicit error(const std::string& msg)
            : std::runtime_error(msg) {}

        /// get the inner exception
        [[nodiscard]] virtual const std::exception& inner() const;
    private:
        std::exception ex;
    };
}
