#include "core/commandpool.hpp"
#include "core/device.hpp"
#include "mini/image.hpp"

#include <vulkan/vulkan_core.h>

#include <string>

namespace Utils {

    using namespace LSFG;

    ///
    /// Upload a DDS file to a Vulkan image.
    ///
    /// @param device The Vulkan device
    /// @param commandPool The command pool
    /// @param image The Vulkan image to upload to
    /// @param path The path to the DDS file.
    ///
    /// @throws std::system_error If the file cannot be opened or read.
    /// @throws ls:vulkan_error If the Vulkan image cannot be created or updated.
    ///
    void uploadImage(const Core::Device& device,
        const Core::CommandPool& commandPool,
        Mini::Image& image, const std::string& path);

}
