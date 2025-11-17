#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstdint>
#include <optional>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a timeline semaphore
    ls::owned_ptr<VkSemaphore> createTimelineSemaphore(const vk::Vulkan& vk, uint32_t initial,
            std::optional<int> fd) {
        VkSemaphore handle{};

        const VkExportSemaphoreCreateInfo exportInfo{
            .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
            .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
        };
        const VkSemaphoreTypeCreateInfo typeInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = fd.has_value() ? &exportInfo : nullptr,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = initial
        };
        const VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &typeInfo,
        };
        auto res = vkCreateSemaphore(vk.dev(), &semaphoreInfo, nullptr, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateSemaphore() failed");

        if (fd.has_value()) {
            // import semaphore from fd
            auto vkImportSemaphoreFdKHR = reinterpret_cast<PFN_vkImportSemaphoreFdKHR>(
                vkGetDeviceProcAddr(vk.dev(), "vkImportSemaphoreFdKHR")); // TODO: cache

            const VkImportSemaphoreFdInfoKHR importInfo{
                .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,
                .semaphore = handle,
                .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT,
                .fd = *fd // closes the fd
            };
            res = vkImportSemaphoreFdKHR(vk.dev(), &importInfo);
            if (res != VK_SUCCESS)
                throw ls::vulkan_error(res, "vkImportSemaphoreFdKHR() failed");
        }

        return ls::owned_ptr<VkSemaphore>(
            new VkSemaphore(handle),
            [dev = vk.dev()](VkSemaphore& semaphore) {
                vkDestroySemaphore(dev, semaphore, nullptr);
            }
        );
    }
}

TimelineSemaphore::TimelineSemaphore(const vk::Vulkan& vk, uint32_t initial, std::optional<int> fd)
    : semaphore(createTimelineSemaphore(vk, initial, fd)) {}

void TimelineSemaphore::signal(const vk::Vulkan& vk, uint64_t value) const {
    const VkSemaphoreSignalInfo signalInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore = *this->semaphore,
        .value = value
    };
    auto res = vkSignalSemaphore(vk.dev(), &signalInfo);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "vkSignalSemaphore() failed");
}

bool TimelineSemaphore::wait(const vk::Vulkan& vk, uint64_t value, uint64_t timeout) const {
    VkSemaphore semaphore = *this->semaphore;
    const VkSemaphoreWaitInfo waitInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &semaphore,
        .pValues = &value
    };
    auto res = vkWaitSemaphores(vk.dev(), &waitInfo, timeout);
    if (res != VK_SUCCESS && res != VK_TIMEOUT)
        throw ls::vulkan_error(res, "vkWaitSemaphores() failed");

    return res == VK_SUCCESS;
}
