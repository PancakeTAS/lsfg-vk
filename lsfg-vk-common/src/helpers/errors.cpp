#include "lsfg-vk-common/helpers/errors.hpp"

#include <vulkan/vulkan_core.h>

using namespace ls;

VkResult vulkan_error::error() const {
    return this->result;
}
