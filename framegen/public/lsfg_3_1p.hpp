#pragma once

#include <vulkan/vulkan_core.h>

#include <functional>
#include <cstdint>
#include <string>
#include <vector>

namespace LSFG_3_1P {

    ///
    /// Initialize the LSFG library.
    ///
    /// @param deviceUUID The UUID of the Vulkan device to use.
    /// @param isHdr Whether the images are in HDR format.
    /// @param flowScale Internal flow scale factor.
    /// @param generationCount Number of frames to generate.
    /// @param loader Function to load shader source code by name.
    ///
    /// @throws LSFG::vulkan_error if Vulkan objects fail to initialize.
    ///
    void initialize(uint64_t deviceUUID,
        bool isHdr, float flowScale, uint64_t generationCount,
        const std::function<std::vector<uint8_t>(const std::string&)>& loader);

    ///
    /// Create a new LSFG context on a swapchain.
    ///
    /// @param in0 File descriptor for the first input image.
    /// @param in1 File descriptor for the second input image.
    /// @param outN File descriptor for each output image. This defines the LSFG level.
    /// @param extent The size of the images
    /// @param format The format of the images.
    /// @return A unique identifier for the created context.
    ///
    /// @throws LSFG::vulkan_error if the context cannot be created.
    ///
    int32_t createContext(
        int in0, int in1, const std::vector<int>& outN,
        VkExtent2D extent, VkFormat format);

    ///
    /// Present a context.
    ///
    /// @param id Unique identifier of the context to present.
    /// @param inSem Semaphore to wait on before starting the generation.
    /// @param outSem Semaphores to signal once each output image is ready.
    ///
    /// @throws LSFG::vulkan_error if the context cannot be presented.
    ///
    void presentContext(int32_t id, int inSem, const std::vector<int>& outSem);

    ///
    /// Delete an LSFG context.
    ///
    /// @param id Unique identifier of the context to delete.
    ///
    void deleteContext(int32_t id);

    ///
    /// Deinitialize the LSFG library.
    ///
    void finalize();

}
