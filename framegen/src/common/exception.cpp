#include "common/exception.hpp"

#include <vulkan/vulkan_core.h>

#include <exception>
#include <stdexcept>
#include <cstdint>
#include <format>
#include <string>

using namespace LSFG;

vulkan_error::vulkan_error(VkResult result, const std::string& message)
    : std::runtime_error(std::format("{} (error {})", message, static_cast<int32_t>(result))),
      result(result) {}

vulkan_error::~vulkan_error() noexcept = default;

rethrowable_error::rethrowable_error(const std::string& message, const std::exception& exe)
        : std::runtime_error(message) {
    this->message = std::format("{}\n- {}", message, exe.what());
}

rethrowable_error::~rethrowable_error() noexcept = default;
