#pragma once

#include "hooks.hpp"
#include "mini/commandbuffer.hpp"
#include "mini/commandpool.hpp"
#include "mini/image.hpp"
#include "mini/semaphore.hpp"

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

///
/// This class is the frame generation context. There should be one instance per swapchain.
///
class LsContext {
public:
    ///
    /// Create the swapchain context.
    ///
    /// @param info The device information to use.
    /// @param swapchain The Vulkan swapchain to use.
    /// @param extent The extent of the swapchain images.
    /// @param swapchainImages The swapchain images to use.
    ///
    /// @throws LSFG::vulkan_error if any Vulkan call fails.
    ///
    LsContext(const Hooks::DeviceInfo& info, VkSwapchainKHR swapchain,
        VkExtent2D extent, const std::vector<VkImage>& swapchainImages);

    ///
    /// Custom present logic.
    ///
    /// @param info The device information to use.
    /// @param pNext Unknown pointer set in the present info structure.
    /// @param queue The Vulkan queue to present the frame on.
    /// @param gameRenderSemaphores The semaphores to wait on before presenting.
    /// @param presentIdx The index of the swapchain image to present.
    /// @return The result of the Vulkan present operation, which can be VK_SUCCESS or VK_SUBOPTIMAL_KHR.
    ///
    /// @throws LSFG::vulkan_error if any Vulkan call fails.
    ///
    VkResult present(const Hooks::DeviceInfo& info, const void* pNext, VkQueue queue,
        const std::vector<VkSemaphore>& gameRenderSemaphores, uint32_t presentIdx);

    // Non-copyable, trivially moveable and destructible
    LsContext(const LsContext&) = delete;
    LsContext& operator=(const LsContext&) = delete;
    LsContext(LsContext&&) = default;
    LsContext& operator=(LsContext&&) = default;
    ~LsContext() = default;
private:
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkExtent2D extent;

    std::shared_ptr<int32_t> lsfgCtxId; // lsfg context id
    Mini::Image frame_0, frame_1; // frames shared with lsfg. write to frame_0 when fc % 2 == 0
    std::vector<Mini::Image> out_n; // output images shared with lsfg, indexed by framegen id

    Mini::CommandPool cmdPool;
    uint64_t frameIdx{0};

    struct RenderPassInfo {
        Mini::CommandBuffer preCopyBuf; // copy from swapchain image to frame_0/frame_1
        std::array<Mini::Semaphore, 2> preCopySemaphores; // signal when preCopyBuf is done

        std::vector<Mini::Semaphore> renderSemaphores; // signal when lsfg is done with frame n

        std::vector<Mini::Semaphore> acquireSemaphores; // signal for swapchain image n

        std::vector<Mini::CommandBuffer> postCopyBufs; // copy from out_n to swapchain image
        std::vector<Mini::Semaphore> postCopySemaphores; // signal when postCopyBuf is done
        std::vector<Mini::Semaphore> prevPostCopySemaphores; // signal for previous postCopyBuf
    }; // data for a single render pass
    std::array<RenderPassInfo, 8> passInfos; // allocate 8 because why not
};
