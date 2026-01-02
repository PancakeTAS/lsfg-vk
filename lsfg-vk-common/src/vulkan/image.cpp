/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <bitset>
#include <optional>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /// create a image
    ls::owned_ptr<VkImage> createImage(const vk::Vulkan& vk,
            VkExtent2D extent, VkFormat format, VkImageUsageFlags usage,
            bool external, VkImageCreateFlags flags, const void* chain) {
        VkImage handle{};

        const VkExternalMemoryImageCreateInfo externalInfo{
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
            .pNext = chain,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
        };
        const VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = external ? &externalInfo : chain,
            .flags = flags,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = {
                .width = extent.width,
                .height = extent.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };
        auto res = vk.df().CreateImage(vk.dev(), &imageInfo, VK_NULL_HANDLE, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateImage() failed");

        return ls::owned_ptr<VkImage>(
            new VkImage(handle),
            [dev = vk.dev(), defunc = vk.df().DestroyImage](VkImage& image) {
                defunc(dev, image, VK_NULL_HANDLE);
            }
        );
    }
    /// allocate memory for a image
    ls::owned_ptr<VkDeviceMemory> allocateMemory(const vk::Vulkan& vk, VkImage image,
            std::optional<int> importFd, std::optional<int*> exportFd) {
        VkDeviceMemory handle{};

        VkMemoryRequirements reqs{};
        vk.df().GetImageMemoryRequirements(vk.dev(), image, &reqs);

        auto mti = vk.findMemoryTypeIndex(
            reqs.memoryTypeBits,
            false
        );
        if (!mti.has_value())
            throw ls::vulkan_error("no suitable memory type found for image");

        const VkMemoryDedicatedAllocateInfoKHR dedicatedInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
            .image = image,
        };
        const VkImportMemoryFdInfoKHR importInfo{
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
            .pNext = &dedicatedInfo,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
            .fd = importFd.value_or(-1)
        };
        const VkExportMemoryAllocateInfo exportInfo{
            .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
            .pNext = &dedicatedInfo,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
        };
        const void* pNextAlloc{};
        if (importFd.has_value())
            pNextAlloc = &importInfo;
        else if (exportFd.has_value())
            pNextAlloc = &exportInfo;
        const VkMemoryAllocateInfo memoryInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = pNextAlloc,
            .allocationSize = reqs.size,
            .memoryTypeIndex = *mti
        };
        auto res = vk.df().AllocateMemory(vk.dev(), &memoryInfo, VK_NULL_HANDLE, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkAllocateMemory() failed");

        res = vk.df().BindImageMemory(vk.dev(), image, handle, 0);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkBindImageMemory() failed");

        if (exportFd.has_value()) {
            const VkMemoryGetFdInfoKHR fdInfo{
                .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
                .memory = handle,
                .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
            };
            int fd{};
            res = vk.df().GetMemoryFdKHR(vk.dev(), &fdInfo, &fd);
            if (res != VK_SUCCESS)
                throw ls::vulkan_error(res, "vkGetMemoryFdKHR() failed");
            **exportFd = fd;
        }

        return ls::owned_ptr<VkDeviceMemory>(
            new VkDeviceMemory(handle),
            [dev = vk.dev(), defunc = vk.df().FreeMemory](VkDeviceMemory& memory) {
                defunc(dev, memory, VK_NULL_HANDLE);
            }
        );
    }
    /// create an image view
    ls::owned_ptr<VkImageView> createImageView(const vk::Vulkan& vk,
            VkImage image, VkFormat format) {
        VkImageView handle{};

        const VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        auto res = vk.df().CreateImageView(vk.dev(), &viewInfo, VK_NULL_HANDLE, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkCreateImageView() failed");

        return ls::owned_ptr<VkImageView>(
            new VkImageView(handle),
            [dev = vk.dev(), defunc = vk.df().DestroyImageView](VkImageView& view) {
                defunc(dev, view, VK_NULL_HANDLE);
            }
        );
    }
}

Image::Image(const vk::Vulkan& vk,
            VkExtent2D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            std::optional<int> importFd,
            std::optional<int*> exportFd,
            VkImageCreateFlags flags, const void* chain) :
        image(createImage(vk,
            extent, format, usage,
            importFd.has_value() || exportFd.has_value(),
            flags, chain
        )),
        memory(allocateMemory(vk,
            *this->image,
            importFd, exportFd
        )),
        view(createImageView(vk,
            *this->image,
            format
        )),
        extent(extent) {
}
