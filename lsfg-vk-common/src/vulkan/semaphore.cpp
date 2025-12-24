/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/vulkan/semaphore.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <optional>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a semaphore
    ls::owned_ptr<VkSemaphore> createSemaphore(const vk::Vulkan& vk, std::optional<int> fd) {
        VkSemaphore handle{};

        const VkExportSemaphoreCreateInfo exportInfo{
            .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
            .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
        };
        const VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = fd.has_value() ? &exportInfo : nullptr
        };
        auto res = vk.df().CreateSemaphore(vk.dev(), &semaphoreInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateSemaphore() failed");

        if (fd.has_value()) {
            // import semaphore from fd
            const VkImportSemaphoreFdInfoKHR importInfo{
                .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,
                .semaphore = handle,
                .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT,
                .fd = *fd // closes the fd
            };
            res = vk.df().ImportSemaphoreFdKHR(vk.dev(), &importInfo);
            if (res != VK_SUCCESS)
                throw ls::vulkan_error(res, "vkImportSemaphoreFdKHR() failed");
        }

        return ls::owned_ptr<VkSemaphore>(
            new VkSemaphore(handle),
            [dev = vk.dev(), defunc = vk.df().DestroySemaphore](VkSemaphore& semaphore) {
                defunc(dev, semaphore, nullptr);
            }
        );
    }
}

Semaphore::Semaphore(const vk::Vulkan& vk, std::optional<int> fd)
    : semaphore(createSemaphore(vk, fd)) {}
