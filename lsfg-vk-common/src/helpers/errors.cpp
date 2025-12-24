/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/helpers/errors.hpp"

#include <exception>

#include <vulkan/vulkan_core.h>

using namespace ls;

VkResult vulkan_error::error() const {
    return this->result;
}

const std::exception& error::inner() const {
    return this->ex;
}
