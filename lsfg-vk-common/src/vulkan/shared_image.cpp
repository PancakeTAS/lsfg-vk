/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/vulkan/shared_image.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace vk;

namespace {
    /* EXPORTABLE - DMA-BUF */

    /// create an image that can be imported from a dma-buf fd
    ls::owned_ptr<VkImage> createDMAImportableImage(const vk::Vulkan& vk,
            VkExtent2D extent, VkFormat format, VkImageUsageFlags usage,
            uint64_t drmModifier,
            const std::vector<std::pair<uint64_t, uint64_t>>& drmLayouts) {
        VkImage handle{};

        // create VkImage
        std::vector<VkSubresourceLayout> layouts(drmLayouts.size());
        for (size_t i = 0; i < layouts.size(); i++)
            layouts[i] = VkSubresourceLayout {
                .offset = drmLayouts.at(i).first,
                .rowPitch = drmLayouts.at(i).second
            };
        const VkImageDrmFormatModifierExplicitCreateInfoEXT explicitModifierInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,
            .drmFormatModifier = drmModifier,
            .drmFormatModifierPlaneCount = 1,
            .pPlaneLayouts = layouts.data()
        };
        const VkExternalMemoryImageCreateInfo externalInfo{
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
            .pNext = &explicitModifierInfo,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
        };
        const VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &externalInfo,
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
            .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
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
    /// import image memory from a dma-buf fd
    ls::owned_ptr<VkDeviceMemory> importDMADeviceMemory(const vk::Vulkan& vk, VkImage image, int fd) {
        VkDeviceMemory handle{};

        // find suitable memory type
        VkMemoryFdPropertiesKHR propsFd{
            .sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR
        };
        auto res = vk.df().GetMemoryFdPropertiesKHR(vk.dev(),
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, fd, &propsFd);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkGetMemoryFdPropertiesKHR() failed");

        VkMemoryRequirements reqs{};
        vk.df().GetImageMemoryRequirements(vk.dev(), image, &reqs);

        reqs.memoryTypeBits &= propsFd.memoryTypeBits;

        auto mti = vk.findMemoryTypeIndex(
            reqs.memoryTypeBits,
            false
        );
        if (!mti.has_value())
            throw ls::vulkan_error(VK_ERROR_UNKNOWN, "no suitable memory type found");

        // create VkDeviceMemory
        const VkImportMemoryFdInfoKHR importInfo{
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
            .fd = fd
        };
        const VkMemoryAllocateInfo memoryInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &importInfo,
            .allocationSize = reqs.size,
            .memoryTypeIndex = *mti
        };
        res = vk.df().AllocateMemory(vk.dev(), &memoryInfo, VK_NULL_HANDLE, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkAllocateMemory() failed");

        // bind memory to image
        res = vk.df().BindImageMemory(vk.dev(), image, handle, 0);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkBindImageMemory() failed");

        return ls::owned_ptr<VkDeviceMemory>(
            new VkDeviceMemory(handle),
            [dev = vk.dev(), defunc = vk.df().FreeMemory](VkDeviceMemory& memory) {
                defunc(dev, memory, VK_NULL_HANDLE);
            }
        );
    }

    /* IMPORTABLE - DMA-BUF */

    /// create an image that can be exported to a dma-buf fd
    ls::owned_ptr<VkImage> createDMAExportableImage(const vk::Vulkan& vk,
            VkExtent2D extent, VkFormat format, VkImageUsageFlags usage,
            const std::vector<uint64_t>& drmModifiers) {
        VkImage handle{};

        // create VkImage
        const VkImageDrmFormatModifierListCreateInfoEXT explicitModifierInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT,
            .drmFormatModifierCount = static_cast<uint32_t>(drmModifiers.size()),
            .pDrmFormatModifiers = drmModifiers.data()
        };
        const VkExternalMemoryImageCreateInfo externalInfo{
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
            .pNext = &explicitModifierInfo,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
        };
        const VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &externalInfo,
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
            .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
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
    /// export image memory to a dma-buf fd
    ls::owned_ptr<VkDeviceMemory> exportDMADeviceMemory(const vk::Vulkan& vk, VkImage image, int& fd) {
        VkDeviceMemory handle{};

        // find suitable memory type
        VkMemoryRequirements reqs{};
        vk.df().GetImageMemoryRequirements(vk.dev(), image, &reqs);

        auto mti = vk.findMemoryTypeIndex(
            reqs.memoryTypeBits,
            false
        );
        if (!mti.has_value())
            throw ls::vulkan_error(VK_ERROR_UNKNOWN, "no suitable memory type found");

        // create VkDeviceMemory
        const VkExportMemoryAllocateInfoKHR importInfo{
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
        };
        const VkMemoryAllocateInfo memoryInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &importInfo,
            .allocationSize = reqs.size,
            .memoryTypeIndex = *mti
        };
        auto res = vk.df().AllocateMemory(vk.dev(), &memoryInfo, VK_NULL_HANDLE, &handle);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkAllocateMemory() failed");

        // bind memory to image
        res = vk.df().BindImageMemory(vk.dev(), image, handle, 0);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkBindImageMemory() failed");

        // export dma-buf fd
        const VkMemoryGetFdInfoKHR exportInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
            .memory = handle,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
        };
        res = vk.df().GetMemoryFdKHR(vk.dev(), &exportInfo, &fd);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkGetMemoryFdKHR() failed");

        return ls::owned_ptr<VkDeviceMemory>(
            new VkDeviceMemory(handle),
            [dev = vk.dev(), defunc = vk.df().FreeMemory](VkDeviceMemory& memory) {
                defunc(dev, memory, VK_NULL_HANDLE);
            }
        );
    }

    /* GENERAL */

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
                .levelCount = 1,
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
    /// get the drm format modifier of an image
    uint64_t getImageDrmModifier(const vk::Vulkan& vk, VkImage image) {
        VkImageDrmFormatModifierPropertiesEXT props{
            .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT
        };
        auto res = vk.df().GetImageDrmFormatModifierPropertiesEXT(vk.dev(), image, &props);
        if (res != VK_SUCCESS)
            throw ls::vulkan_error(res, "vkGetImageDrmFormatModifierPropertiesEXT() failed");

        return props.drmFormatModifier;
    }
    /// get the drm offsets and row pitches of an image
    std::vector<std::pair<uint64_t, uint64_t>> getImageLayouts(const vk::Vulkan& vk,
            VkImage image, uint64_t drmModifier) {
        std::vector<std::pair<uint64_t, uint64_t>> result;

        // fetch drm modifier information
        VkDrmFormatModifierPropertiesList2EXT formats{
            .sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_2_EXT,
        };
        VkPhysicalDeviceProperties2 props{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &formats
        };
        vk.fi().GetPhysicalDeviceProperties2(vk.physdev(), &props);

        std::vector<VkDrmFormatModifierProperties2EXT> formatProps(formats.drmFormatModifierCount);
        formats.pDrmFormatModifierProperties = formatProps.data();

        vk.fi().GetPhysicalDeviceProperties2(vk.physdev(), &props);

        // find plane count for the modifier
        std::optional<size_t> planeCount;
        for (const auto& fp : formatProps) {
            if (fp.drmFormatModifier != drmModifier)
                continue;

            planeCount.emplace(fp.drmFormatModifierPlaneCount);
        }

        if (!planeCount.has_value())
            throw ls::vulkan_error(VK_ERROR_UNKNOWN, "invalid drm modifier");

        // get offsets and row pitches
        const std::vector<VkImageAspectFlagBits> bits{
            VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT,
            VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT,
            VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT,
            VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT,
        };

        result.reserve(*planeCount);
        for (size_t i = 0; i < *planeCount; i++) {
            VkSubresourceLayout layout{};

            const VkImageSubresource subresource{
                .aspectMask = bits.at(i),
            };
            vk.df().GetImageSubresourceLayout(vk.dev(), image, &subresource, &layout);

            result.emplace_back(
                layout.offset,
                layout.rowPitch
            );
        }

        return result;
    }
}

SharedImage::SharedImage(const vk::Vulkan& vk,
            VkExtent2D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            const std::vector<uint64_t>& drmModifiers,
            int& fd) :
        image(createDMAExportableImage(vk,
            extent, format, usage,
            drmModifiers
        )),
        memory(exportDMADeviceMemory(vk,
            *this->image,
            fd
        )),
        view(createImageView(vk,
            *this->image,
            format
        )),
        extent(extent),
        modifier(getImageDrmModifier(vk, *this->image)),
        layouts(getImageLayouts(vk, *this->image, this->modifier)) {
}


SharedImage::SharedImage(const vk::Vulkan& vk,
            VkExtent2D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            uint64_t drmModifier,
            const std::vector<std::pair<uint64_t, uint64_t>>& drmLayouts,
            int fd) :
        image(createDMAImportableImage(vk,
            extent, format, usage,
            drmModifier, drmLayouts
        )),
        memory(importDMADeviceMemory(vk,
            *this->image,
            fd
        )),
        view(createImageView(vk,
            *this->image,
            format
        )),
        extent(extent),
        modifier(getImageDrmModifier(vk, *this->image)),
        layouts(getImageLayouts(vk, *this->image, this->modifier)) {
}
