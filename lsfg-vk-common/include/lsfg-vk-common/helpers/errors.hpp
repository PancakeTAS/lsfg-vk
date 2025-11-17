#pragma once

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
}
