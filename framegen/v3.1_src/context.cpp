#include "v3_1/context.hpp"
#include "common/utils.hpp"
#include "common/exception.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>
#include <cstddef>
#include <algorithm>
#include <optional>
#include <cstdint>

using namespace LSFG_3_1;

Context::Context(Vulkan& vk,
        int in0, int in1, const std::vector<int>& outN,
        VkExtent2D extent, VkFormat format) {
    // import input images
    this->inImg_0 = Core::Image(vk.device, extent, format,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, in0);
    this->inImg_1 = Core::Image(vk.device, extent, format,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, in1);

    // prepare render data
    for (size_t i = 0; i < 8; i++) {
        auto& data = this->data.at(i);
        data.internalSemaphores.resize(vk.generationCount);
        data.outSemaphores.resize(vk.generationCount);
        data.completionFences.resize(vk.generationCount);
        data.cmdBuffers2.resize(vk.generationCount);
    }

    // create shader chains
    this->mipmaps = Shaders::Mipmaps(vk, this->inImg_0, this->inImg_1);
    for (size_t i = 0; i < 7; i++)
        this->alpha.at(i) = Shaders::Alpha(vk, this->mipmaps.getOutImages().at(i));
    this->beta = Shaders::Beta(vk, this->alpha.at(0).getOutImages());
    for (size_t i = 0; i < 7; i++) {
        this->gamma.at(i) = Shaders::Gamma(vk,
            this->alpha.at(6 - i).getOutImages(),
            this->beta.getOutImages().at(std::min<size_t>(6 - i, 5)),
            (i == 0) ? std::nullopt : std::make_optional(this->gamma.at(i - 1).getOutImage()));
        if (i < 4) continue;

        this->delta.at(i - 4) = Shaders::Delta(vk,
            this->alpha.at(6 - i).getOutImages(),
            this->beta.getOutImages().at(6 - i),
            (i == 4) ? std::nullopt : std::make_optional(this->gamma.at(i - 1).getOutImage()),
            (i == 4) ? std::nullopt : std::make_optional(this->delta.at(i - 5).getOutImage1()),
            (i == 4) ? std::nullopt : std::make_optional(this->delta.at(i - 5).getOutImage2()));
    }
    this->generate = Shaders::Generate(vk,
        this->inImg_0, this->inImg_1,
        this->gamma.at(6).getOutImage(),
        this->delta.at(2).getOutImage1(),
        this->delta.at(2).getOutImage2(),
        outN, format);
}

void Context::present(Vulkan& vk,
        int inSem, const std::vector<int>& outSem) {
    auto& data = this->data.at(this->frameIdx % 8);

    // 3. wait for completion of previous frame in this slot
    if (data.shouldWait)
        for (auto& fence : data.completionFences)
            if (!fence.wait(vk.device, UINT64_MAX))
                throw LSFG::vulkan_error(VK_TIMEOUT, "Fence wait timed out");
    data.shouldWait = true;

    // 1. create mipmaps and process input image
    if (inSem >= 0) data.inSemaphore = Core::Semaphore(vk.device, inSem);
    for (size_t i = 0; i < vk.generationCount; i++)
        data.internalSemaphores.at(i) = Core::Semaphore(vk.device);

    data.cmdBuffer1 = Core::CommandBuffer(vk.device, vk.commandPool);
    data.cmdBuffer1.begin();

    this->mipmaps.Dispatch(data.cmdBuffer1, this->frameIdx);
    for (size_t i = 0; i < 7; i++)
        this->alpha.at(6 - i).Dispatch(data.cmdBuffer1, this->frameIdx);
    this->beta.Dispatch(data.cmdBuffer1, this->frameIdx);

    data.cmdBuffer1.end();
    std::vector<Core::Semaphore> waits = { data.inSemaphore };
    if (inSem < 0) waits.clear();
    data.cmdBuffer1.submit(vk.device.getComputeQueue(), std::nullopt,
        waits, std::nullopt,
        data.internalSemaphores, std::nullopt);

    // 2. generate intermediary frames
    for (size_t pass = 0; pass < vk.generationCount; pass++) {
        auto& internalSemaphore = data.internalSemaphores.at(pass);
        auto& outSemaphore = data.outSemaphores.at(pass);
        if (inSem >= 0) outSemaphore = Core::Semaphore(vk.device, outSem.empty() ? -1 : outSem.at(pass));
        auto& completionFence = data.completionFences.at(pass);
        completionFence = Core::Fence(vk.device);

        auto& buf2 = data.cmdBuffers2.at(pass);
        buf2 = Core::CommandBuffer(vk.device, vk.commandPool);
        buf2.begin();

        for (size_t i = 0; i < 7; i++) {
            this->gamma.at(i).Dispatch(buf2, this->frameIdx, pass);
            if (i >= 4)
                this->delta.at(i - 4).Dispatch(buf2, this->frameIdx, pass);
        }
        this->generate.Dispatch(buf2, this->frameIdx, pass);

        buf2.end();
        std::vector<Core::Semaphore> signals = { outSemaphore };
        if (inSem < 0) signals.clear();
        buf2.submit(vk.device.getComputeQueue(), completionFence,
            { internalSemaphore }, std::nullopt,
            signals, std::nullopt);
    }

    this->frameIdx++;
}
