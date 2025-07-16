#include "v3_1p/shaders/beta.hpp"
#include "common/utils.hpp"
#include "core/commandbuffer.hpp"
#include "core/image.hpp"

#include <vulkan/vulkan_core.h>

#include <array>
#include <utility>
#include <cstddef>
#include <cstdint>

using namespace LSFG_3_1P::Shaders;

Beta::Beta(Vulkan& vk, std::array<std::array<Core::Image, 2>, 3> inImgs)
        : inImgs(std::move(inImgs)) {
    // create resources
    this->shaderModules = {{
        vk.shaders.getShader(vk.device, "p_beta[0]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_beta[1]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_beta[2]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_beta[3]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_beta[4]",
            { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } })
    }};
    this->pipelines = {{
        vk.shaders.getPipeline(vk.device, "p_beta[0]"),
        vk.shaders.getPipeline(vk.device, "p_beta[1]"),
        vk.shaders.getPipeline(vk.device, "p_beta[2]"),
        vk.shaders.getPipeline(vk.device, "p_beta[3]"),
        vk.shaders.getPipeline(vk.device, "p_beta[4]")
    }};
    this->samplers.at(0) = vk.resources.getSampler(vk.device);
    this->samplers.at(1) = vk.resources.getSampler(vk.device,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, true);
    for (size_t i = 0; i < 3; i++)
        this->firstDescriptorSet.at(i) = Core::DescriptorSet(vk.device, vk.descriptorPool, this->shaderModules.at(0));
    for (size_t i = 0; i < 4; i++)
        this->descriptorSets.at(i) = Core::DescriptorSet(vk.device, vk.descriptorPool, this->shaderModules.at(i + 1));
    this->buffer = vk.resources.getBuffer(vk.device, 0.5F);

    // create internal images/outputs
    const VkExtent2D extent = this->inImgs.at(0).at(0).getExtent();
    for (size_t i = 0; i < 2; i++) {
        this->tempImgs1.at(i) = Core::Image(vk.device, extent);
        this->tempImgs2.at(i) = Core::Image(vk.device, extent);
    }

    for (size_t i = 0; i < 6; i++)
        this->outImgs.at(i) = Core::Image(vk.device,
            { extent.width >> i, extent.height >> i },
            VK_FORMAT_R8_UNORM);

    // hook up shaders
    for (size_t i = 0; i < 3; i++) {
        this->firstDescriptorSet.at(i).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(1))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs.at((i + 1) % 3))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs.at((i + 2) % 3))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs.at(i % 3))
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
            .build();
    }
    this->descriptorSets.at(0).update(vk.device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
        .build();
    this->descriptorSets.at(1).update(vk.device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
        .build();
    this->descriptorSets.at(2).update(vk.device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
        .build();
    this->descriptorSets.at(3).update(vk.device)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs)
        .build();
}

void Beta::Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount) {
    // first pass
    const auto extent = this->tempImgs1.at(0).getExtent();
    uint32_t threadsX = (extent.width + 7) >> 3;
    uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImgs.at(0))
        .addW2R(this->inImgs.at(1))
        .addW2R(this->inImgs.at(2))
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(0).bind(buf);
    this->firstDescriptorSet.at(frameCount % 3).bind(buf, this->pipelines.at(0));
    buf.dispatch(threadsX, threadsY, 1);

    // second pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(1).bind(buf);
    this->descriptorSets.at(0).bind(buf, this->pipelines.at(1));
    buf.dispatch(threadsX, threadsY, 1);

    // third pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(2).bind(buf);
    this->descriptorSets.at(1).bind(buf, this->pipelines.at(2));
    buf.dispatch(threadsX, threadsY, 1);

    // fourth pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(3).bind(buf);
    this->descriptorSets.at(2).bind(buf, this->pipelines.at(3));
    buf.dispatch(threadsX, threadsY, 1);

    // fifth pass
    threadsX = (extent.width + 31) >> 5;
    threadsY = (extent.height + 31) >> 5;

    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->outImgs)
        .build();

    this->pipelines.at(4).bind(buf);
    this->descriptorSets.at(3).bind(buf, this->pipelines.at(4));
    buf.dispatch(threadsX, threadsY, 1);
}
