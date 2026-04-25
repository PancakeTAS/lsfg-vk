/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk/lsfgvk.hpp"
#include "lsfgvk.hpp"
#include "modules/library.hpp"
#include "modules/pipeline.hpp"
#include "utility/pipelines.hpp"
#include "utility/vkhelper.hpp"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

using namespace lsfgvk;

Instance::Instance(
    const std::string& deviceId,
    const std::filesystem::path& lsfgvkDllPath,
    bool allowFP16
) {
    // Create Vulkan context
    vk::detail::DispatchLoaderDynamic dld{};

    auto instance{vkhelper::createInstance(dld)};
    auto physdev{vkhelper::findPhysicalDevice(dld, *instance, deviceId)};

    const uint32_t qfi{vkhelper::findComputeQueueFamilyIndex(dld, physdev)};
    const bool fp16{allowFP16 && vkhelper::checkHalfPrecisionSupport(dld, physdev)};

    auto [device, queue] = vkhelper::createDevice(dld, physdev, qfi, fp16);

    // Construct instance
    library::ShaderLibrary library{
        dld,
        *device,
        fp16,
        lsfgvkDllPath
    };

    this->m_priv = std::make_unique<priv::Instance>(priv::Instance {
        .vk = {
            .dld = dld,
            .instance = std::move(instance),
            .physdev = physdev,
            .device = std::move(device),
            .queue = queue,
            .qfi = qfi,
            .fp16 = fp16
        },
        .shaderLibrary = std::move(library)
    });
}

Context::Context(
    const Instance& instance,
    uint32_t width,
    uint32_t height,
    float flowScale,
    bool performanceMode
) {
    const auto& vk{instance.m_priv->vk};

    pipeline::Pipeline pipeline{
        vk.dld,
        *vk.device,
        vk.physdev,
        vk.queue,
        vk.qfi,
        instance.m_priv->shaderLibrary,
        lsfgvk::getPipelineSignature(performanceMode),
        { width, height },
        flowScale,
        performanceMode,
        false /* TODO: HDR */
    };

    this->m_priv = std::make_unique<priv::Context>(priv::Context {
        .instance = std::ref(*instance.m_priv),
        .pipeline = std::move(pipeline),
        .syncSemaphore = { vkhelper::createTimelineSemaphore(vk.dld, *vk.device, true), 0 },
        .internalSemaphores = { vkhelper::createTimelineSemaphore(vk.dld, *vk.device), 0 },
        .fence = vkhelper::createFence(vk.dld, *vk.device),
    });
}

FileDescriptors Context::exportFds() const {
    const auto& vk{this->m_priv->instance.get().vk};
    const auto& pipeline{this->m_priv->pipeline};

    return{
        .sourceFd = vkhelper::exportMemoryFd(
            vk.dld, *vk.device,
            pipeline.getExternalInputs().front().memory
        ),
        .destinationFd = vkhelper::exportMemoryFd(
            vk.dld, *vk.device,
            pipeline.getExternalOutputs().front().memory
        ),
        .syncFd = vkhelper::exportSemaphoreFd(
            vk.dld, *vk.device,
            *this->m_priv->syncSemaphore.first
        )
    };
}

void Context::dispatch(uint32_t total) {
    auto& ctx{*this->m_priv};
    const auto& vk{ctx.instance.get().vk};

    // Increment iteration counter after previous frame is completed
    auto* mapped{ctx.pipeline.getMappedBuffer()};
    if (ctx.firstIteration) {
        ctx.firstIteration = false;
        mapped->iteration = 0;
    } else {
        if (vk.device->waitForFences(*ctx.fence, true, UINT64_MAX, vk.dld) != vk::Result::eSuccess)
            throw std::runtime_error("Unable to wait for completion of previous iteration");
        vk.device->resetFences(*ctx.fence, vk.dld);
        mapped->iteration++;
    }

    const auto& cmdbufs{ctx.pipeline.getCmdbufs()};

    // Dispatch pre-pass
    auto& sync{ctx.syncSemaphore};
    sync.second++;

    auto& internal{ctx.internalSemaphores};
    internal.second++;

    vk::TimelineSemaphoreSubmitInfo timelineInfo{
        .waitSemaphoreValueCount = 1,
        .pWaitSemaphoreValues = &sync.second,
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = &internal.second
    };

    const vk::PipelineStageFlags waitStage{vk::PipelineStageFlagBits::eTopOfPipe};
    vk.queue.submit(
        {{
            .pNext = &timelineInfo,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*sync.first,
            .pWaitDstStageMask = &waitStage,
            .commandBufferCount = 1U,
            .pCommandBuffers = &*cmdbufs.at(0),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*internal.first
        }},
        nullptr,
        vk.dld
    );

    // Dispatch main passes
    uint64_t prevInternal{};
    for (uint32_t i = 0; i < total; i++) {
        const auto& transCmdbuf{ctx.pipeline.getCmdbufs().at(0)}; // FIXME: replace with actual buf

        // Transition command buffer to next timestamp
        if (i == 0) {
            prevInternal = internal.second;
            timelineInfo.pWaitSemaphoreValues = &prevInternal;
        } else {
            sync.second++;
            timelineInfo.pWaitSemaphoreValues = &sync.second;
        }

        internal.second++;
        timelineInfo.pSignalSemaphoreValues = &internal.second;

        vk.queue.submit(
            {{
                .pNext = &timelineInfo,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = i == 0 ? &*internal.first : &*sync.first,
                .pWaitDstStageMask = &waitStage,
                .commandBufferCount = 1,
                .pCommandBuffers = &*transCmdbuf,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &*internal.first
            }},
            nullptr,
            vk.dld
        );

        // Dispatch main pass
        timelineInfo.pWaitSemaphoreValues = &internal.second;

        sync.second++;
        timelineInfo.pSignalSemaphoreValues = &sync.second;

        vk.queue.submit(
            {{
                .pNext = &timelineInfo,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*internal.first,
                .pWaitDstStageMask = &waitStage,
                .commandBufferCount = 1,
                .pCommandBuffers = &*cmdbufs.at(1),
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &*sync.first
            }},
            i == (total - 1) ? *ctx.fence : nullptr,
            vk.dld
        );
    }
}

void Context::idle() const {
    const auto& ctx{*this->m_priv};
    const auto& vk{ctx.instance.get().vk};

    vk.device->waitIdle(vk.dld);
}

Context::~Context() {
    try {
        // NOTE: This will freeze if the user didn't signal the sync semaphore high enough to
        // allow the pipeline to complete.
        this->idle();
    } catch (...) { // NOLINT (empty catch)
        // Not much we can do here..
    }
}

Instance::~Instance() = default;
