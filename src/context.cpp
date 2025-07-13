#include "context.hpp"
#include "extract/extract.hpp"
#include "extract/trans.hpp"
#include "utils/utils.hpp"
#include "utils/log.hpp"
#include "hooks.hpp"
#include "layer.hpp"
#include "common/exception.hpp"

#include <vulkan/vulkan_core.h>
#include <lsfg_3_1.hpp>
#include <lsfg_3_1p.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

LsContext::LsContext(const Hooks::DeviceInfo& info, VkSwapchainKHR swapchain,
        VkExtent2D extent, const std::vector<VkImage>& swapchainImages)
        : swapchain(swapchain), swapchainImages(swapchainImages),
          extent(extent) {
    // read environment variables
    const char* lsfgFlowScaleStr = getenv("LSFG_FLOW_SCALE");
    float flowScale = lsfgFlowScaleStr
        ? std::stof(lsfgFlowScaleStr)
        : 1.0F;
    flowScale = std::max(0.25F, std::min(flowScale, 1.0F));

    const char* lsfgHdrStr = getenv("LSFG_HDR");
    const bool isHdr = lsfgHdrStr
        ? *lsfgHdrStr == '1'
        : false;

    const char* lsfgPerfModeStr = getenv("LSFG_PERF_MODE");
    const bool perfMode = lsfgPerfModeStr
        ? *lsfgPerfModeStr == '1'
        : false;

    const char* lsfgFramePacingStr = getenv("LSFG_FRAME_PACING_DELAY");
    const uint32_t framePacingDelay = lsfgFramePacingStr
        ? static_cast<uint32_t>(std::max(0, std::stoi(lsfgFramePacingStr)))
        : 0;

    // we could take the format from the swapchain,
    // but honestly this is safer.
    const VkFormat format = isHdr
        ? VK_FORMAT_R8G8B8A8_UNORM
        : VK_FORMAT_R16G16B16A16_SFLOAT;

    // prepare textures for lsfg
    int frame_0_fd{};
    this->frame_0 = Mini::Image(
        info.device, info.physicalDevice,
        extent, format,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &frame_0_fd);
    Log::info("context", "Created frame_0 image and obtained fd: {}",
        frame_0_fd);

    int frame_1_fd{};
    this->frame_1 = Mini::Image(
        info.device, info.physicalDevice,
        extent, format,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &frame_1_fd);
    Log::info("context", "Created frame_1 image and obtained fd: {}",
        frame_1_fd);

    std::vector<int> out_n_fds(info.frameGen);
    for (size_t i = 0; i < info.frameGen; ++i) {
        this->out_n.emplace_back(
            info.device, info.physicalDevice,
            extent, format,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            &out_n_fds.at(i));
        Log::info("context", "Created out_n[{}] image and obtained fd: {}",
            i, out_n_fds.at(i));
    }

    this->isPerfMode = false;
    this->framePacingDelayMs = framePacingDelay;
    if (this->framePacingDelayMs > 0) {
        Log::info("context", "Frame pacing enabled with {}ms delay per generated frame",
            this->framePacingDelayMs);
    }
    auto* lsfgInitialize = LSFG_3_1::initialize;
    auto* lsfgCreateContext = LSFG_3_1::createContext;
    auto* lsfgDeleteContext = LSFG_3_1::deleteContext;
    if (perfMode) {
        Log::debug("context", "Using performance mode");
        this->isPerfMode = true;
        lsfgInitialize = LSFG_3_1P::initialize;
        lsfgCreateContext = LSFG_3_1P::createContext;
        lsfgDeleteContext = LSFG_3_1P::deleteContext;
    }
    // initialize lsfg
    Log::debug("context", "(entering LSFG initialization)");
    setenv("DISABLE_LSFG", "1", 1); // NOLINT
    Extract::extractShaders();
    lsfgInitialize(
        Utils::getDeviceUUID(info.physicalDevice),
        isHdr, 1.0F / flowScale, info.frameGen,
        [](const std::string& name) {
            auto dxbc = Extract::getShader(name);
            auto spirv = Extract::translateShader(dxbc);
            return spirv;
        }
    );
    unsetenv("DISABLE_LSFG"); // NOLINT
    Log::debug("context", "(exiting LSFG initialization)");

    // create lsfg context
    Log::debug("context", "(entering LSFG context creation)");
    this->lsfgCtxId = std::shared_ptr<int32_t>(
        new int32_t(lsfgCreateContext(frame_0_fd, frame_1_fd, out_n_fds,
            extent, format)),
        [lsfgDeleteContext = lsfgDeleteContext](const int32_t* id) {
            Log::info("context",
                "(entering LSFG context deletion with id: {})", *id);
            lsfgDeleteContext(*id);
            Log::info("context",
                "(exiting LSFG context deletion with id: {})", *id);
        }
    );
    Log::info("context", "(exiting LSFG context creation with id: {})",
        *this->lsfgCtxId);

    // prepare render passes
    this->cmdPool = Mini::CommandPool(info.device, info.queue.first);
    for (size_t i = 0; i < 8; i++) {
        auto& pass = this->passInfos.at(i);
        pass.renderSemaphores.resize(info.frameGen);
        pass.acquireSemaphores.resize(info.frameGen);
        pass.postCopyBufs.resize(info.frameGen);
        pass.postCopySemaphores.resize(info.frameGen);
        pass.prevPostCopySemaphores.resize(info.frameGen);
    }

    Log::info("context", "Remaining misc context init finished successfully.");
}

VkResult LsContext::present(const Hooks::DeviceInfo& info, const void* pNext, VkQueue queue,
        const std::vector<VkSemaphore>& gameRenderSemaphores, uint32_t presentIdx) {
    auto& pass = this->passInfos.at(this->frameIdx % 8);

    // 1. copy swapchain image to frame_0/frame_1
    Log::debug("context2", "1. Copying swapchain image {} to frame {}",
        presentIdx, this->frameIdx % 2 == 0 ? "0" : "1");
    int preCopySemaphoreFd{};
    pass.preCopySemaphores.at(0) = Mini::Semaphore(info.device, &preCopySemaphoreFd);
    pass.preCopySemaphores.at(1) = Mini::Semaphore(info.device);
    pass.preCopyBuf = Mini::CommandBuffer(info.device, this->cmdPool);
    pass.preCopyBuf.begin();

    Utils::copyImage(pass.preCopyBuf.handle(),
        this->swapchainImages.at(presentIdx),
        this->frameIdx % 2 == 0 ? this->frame_0.handle() : this->frame_1.handle(),
        this->extent.width, this->extent.height,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        true, false);

    pass.preCopyBuf.end();

    std::vector<VkSemaphore> gameRenderSemaphores2 = gameRenderSemaphores;
    if (this->frameIdx > 0)
        gameRenderSemaphores2.emplace_back(this->passInfos.at((this->frameIdx - 1) % 8)
            .preCopySemaphores.at(1).handle());
    pass.preCopyBuf.submit(info.queue.second,
        gameRenderSemaphores2,
        { pass.preCopySemaphores.at(0).handle(),
          pass.preCopySemaphores.at(1).handle() });

    // 2. render intermediary frames
    Log::debug("context2", "2. Rendering intermediary frames");
    std::vector<int> renderSemaphoreFds(info.frameGen);
    for (size_t i = 0; i < info.frameGen; ++i)
        pass.renderSemaphores.at(i) = Mini::Semaphore(info.device, &renderSemaphoreFds.at(i));

    Log::debug("context2",
        "(entering LSFG present with id: {})", *this->lsfgCtxId);
    if (this->isPerfMode) {
        LSFG_3_1P::presentContext(*this->lsfgCtxId,
            preCopySemaphoreFd,
            renderSemaphoreFds);
    } else {
        LSFG_3_1::presentContext(*this->lsfgCtxId,
            preCopySemaphoreFd,
            renderSemaphoreFds);
    }
    Log::debug("context2",
        "(exiting LSFG present with id: {})", *this->lsfgCtxId);

    for (size_t i = 0; i < info.frameGen; i++) {
        // 3. acquire next swapchain image
        Log::debug("context2", "3. Acquiring next swapchain image for frame {}", i);
        pass.acquireSemaphores.at(i) = Mini::Semaphore(info.device);
        uint32_t imageIdx{};
        auto res = Layer::ovkAcquireNextImageKHR(info.device, this->swapchain, UINT64_MAX,
            pass.acquireSemaphores.at(i).handle(), VK_NULL_HANDLE, &imageIdx);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw LSFG::vulkan_error(res, "Failed to acquire next swapchain image");

        // 4. copy output image to swapchain image
        Log::debug("context2", "4. Copying output image to swapchain image for frame {}", i);
        pass.postCopySemaphores.at(i) = Mini::Semaphore(info.device);
        pass.prevPostCopySemaphores.at(i) = Mini::Semaphore(info.device);
        pass.postCopyBufs.at(i) = Mini::CommandBuffer(info.device, this->cmdPool);
        pass.postCopyBufs.at(i).begin();

        Utils::copyImage(pass.postCopyBufs.at(i).handle(),
            this->out_n.at(i).handle(),
            this->swapchainImages.at(imageIdx),
            this->extent.width, this->extent.height,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            false, true);

        pass.postCopyBufs.at(i).end();
        pass.postCopyBufs.at(i).submit(info.queue.second,
            { pass.acquireSemaphores.at(i).handle(),
              pass.renderSemaphores.at(i).handle() },
            { pass.postCopySemaphores.at(i).handle(),
              pass.prevPostCopySemaphores.at(i).handle() });

        // 5. present swapchain image
        Log::debug("context2", "5. Presenting swapchain image for frame {}", i);
        std::vector<VkSemaphore> waitSemaphores{ pass.postCopySemaphores.at(i).handle() };
        if (i != 0) waitSemaphores.emplace_back(pass.prevPostCopySemaphores.at(i - 1).handle());

        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = i == 0 ? pNext : nullptr, // only set on first present
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = &this->swapchain,
            .pImageIndices = &imageIdx,
        };
        res = Layer::ovkQueuePresentKHR(queue, &presentInfo);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw LSFG::vulkan_error(res, "Failed to present swapchain image");

        // Apply frame pacing delay if configured
        if (this->framePacingDelayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(this->framePacingDelayMs));
        }
    }

    // 6. present actual next frame
    Log::debug("context2", "6. Presenting actual next frame");
    VkSemaphore lastPrevPostCopySemaphore =
        pass.prevPostCopySemaphores.at(info.frameGen - 1).handle();
    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &lastPrevPostCopySemaphore,
        .swapchainCount = 1,
        .pSwapchains = &this->swapchain,
        .pImageIndices = &presentIdx,
    };
    auto res = Layer::ovkQueuePresentKHR(queue, &presentInfo);
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        throw LSFG::vulkan_error(res, "Failed to present swapchain image");

    this->frameIdx++;
    return res;
}
