#include "v3_1p/shaders/delta.hpp"
#include "common/utils.hpp"
#include "core/commandbuffer.hpp"
#include "core/image.hpp"

#include <vulkan/vulkan_core.h>

#include <array>
#include <optional>
#include <utility>
#include <cstddef>
#include <cstdint>

using namespace LSFG_3_1P::Shaders;

Delta::Delta(Vulkan& vk, std::array<std::array<Core::Image, 2>, 3> inImgs1,
        Core::Image inImg2,
        std::optional<Core::Image> optImg1,
        std::optional<Core::Image> optImg2)
        : inImgs1(std::move(inImgs1)), inImg2(std::move(inImg2)),
          optImg1(std::move(optImg1)), optImg2(std::move(optImg2)) {
    // create resources
    this->shaderModules = {{
        vk.shaders.getShader(vk.device, "p_delta[0]",
            { { 1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[1]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[2]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[3]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[4]",
            { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[5]",
            { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[6]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[7]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[8]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "p_delta[9]",
            { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } })
    }};
    this->pipelines = {{
        vk.shaders.getPipeline(vk.device, "p_delta[0]"),
        vk.shaders.getPipeline(vk.device, "p_delta[1]"),
        vk.shaders.getPipeline(vk.device, "p_delta[2]"),
        vk.shaders.getPipeline(vk.device, "p_delta[3]"),
        vk.shaders.getPipeline(vk.device, "p_delta[4]"),
        vk.shaders.getPipeline(vk.device, "p_delta[5]"),
        vk.shaders.getPipeline(vk.device, "p_delta[6]"),
        vk.shaders.getPipeline(vk.device, "p_delta[7]"),
        vk.shaders.getPipeline(vk.device, "p_delta[8]"),
        vk.shaders.getPipeline(vk.device, "p_delta[9]")
    }};
    this->samplers.at(0) = vk.resources.getSampler(vk.device);
    this->samplers.at(1) = vk.resources.getSampler(vk.device,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, true);
    this->samplers.at(2) = vk.resources.getSampler(vk.device,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_COMPARE_OP_ALWAYS, false);

    // create internal images/outputs
    const VkExtent2D extent = this->inImgs1.at(0).at(0).getExtent();
    for (size_t i = 0; i < 3; i++)
        this->tempImgs1.at(i) = Core::Image(vk.device, extent);
    for (size_t i = 0; i < 2; i++)
        this->tempImgs2.at(i) = Core::Image(vk.device, extent);

    this->outImg1 = Core::Image(vk.device,
        { extent.width, extent.height },
        VK_FORMAT_R16G16B16A16_SFLOAT);
    this->outImg2 = Core::Image(vk.device,
        { extent.width, extent.height },
        VK_FORMAT_R16G16B16A16_SFLOAT);

    // hook up shaders
    for (size_t pass_idx = 0; pass_idx < vk.generationCount; pass_idx++) {
        auto& pass = this->passes.emplace_back();
        pass.buffer = vk.resources.getBuffer(vk.device,
            static_cast<float>(pass_idx + 1) / static_cast<float>(vk.generationCount + 1),
            false, !this->optImg1.has_value());
        for (size_t i = 0; i < 3; i++) {
            pass.firstDescriptorSet.at(i) = Core::DescriptorSet(vk.device, vk.descriptorPool,
                this->shaderModules.at(0));
            pass.firstDescriptorSet.at(i).update(vk.device)
                .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pass.buffer)
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(1))
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(2))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs1.at((i + 2) % 3))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs1.at(i % 3))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
                .build();
        }
        pass.descriptorSets.at(0) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(1));
        pass.descriptorSets.at(0).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
            .build();
        pass.descriptorSets.at(1) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(2));
        pass.descriptorSets.at(1).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(0))
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(1))
            .build();
        pass.descriptorSets.at(2) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(3));
        pass.descriptorSets.at(2).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(1))
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
            .build();
        pass.descriptorSets.at(3) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(4));
        pass.descriptorSets.at(3).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pass.buffer)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(2))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg1)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg2)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImg1)
            .build();
        for (size_t i = 0; i < 3; i++) {
            pass.sixthDescriptorSet.at(i) = Core::DescriptorSet(vk.device, vk.descriptorPool,
                this->shaderModules.at(5));
            pass.sixthDescriptorSet.at(i).update(vk.device)
                .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pass.buffer)
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(1))
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(2))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs1.at((i + 2) % 3))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs1.at(i % 3))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg1)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg2)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2.at(0))
                .build();
        }
        pass.descriptorSets.at(4) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(6));
        pass.descriptorSets.at(4).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2.at(0))
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(0))
            .build();
        pass.descriptorSets.at(5) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(7));
        pass.descriptorSets.at(5).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(0))
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2.at(0))
            .build();
        pass.descriptorSets.at(6) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(8));
        pass.descriptorSets.at(6).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2.at(0))
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(0))
            .build();
        pass.descriptorSets.at(7) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(9));
        pass.descriptorSets.at(7).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pass.buffer)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(2))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImg2)
            .build();
    }
}

void Delta::Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount, uint64_t pass_idx,
        bool last) {
    auto& pass = this->passes.at(pass_idx);

    // first shader
    const auto extent = this->tempImgs1.at(0).getExtent();
    const uint32_t threadsX = (extent.width + 7) >> 3;
    const uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImgs1.at((frameCount + 2) % 3))
        .addW2R(this->inImgs1.at(frameCount % 3))
        .addW2R(this->optImg1)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(0).bind(buf);
    pass.firstDescriptorSet.at(frameCount % 3).bind(buf, this->pipelines.at(0));
    buf.dispatch(threadsX, threadsY, 1);

    // second shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(1).bind(buf);
    pass.descriptorSets.at(0).bind(buf, this->pipelines.at(1));
    buf.dispatch(threadsX, threadsY, 1);

    // third shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(2).bind(buf);
    pass.descriptorSets.at(1).bind(buf, this->pipelines.at(2));
    buf.dispatch(threadsX, threadsY, 1);

    // fourth shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(3).bind(buf);
    pass.descriptorSets.at(2).bind(buf, this->pipelines.at(3));
    buf.dispatch(threadsX, threadsY, 1);

    // fifth shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addW2R(this->optImg1)
        .addW2R(this->inImg2)
        .addR2W(this->outImg1)
        .build();

    this->pipelines.at(4).bind(buf);
    pass.descriptorSets.at(3).bind(buf, this->pipelines.at(4));
    buf.dispatch(threadsX, threadsY, 1);

    // sixth shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->inImgs1.at((frameCount + 2) % 3))
        .addW2R(this->inImgs1.at(frameCount % 3))
        .addW2R(this->optImg1)
        .addW2R(this->optImg2)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(5).bind(buf);
    pass.sixthDescriptorSet.at(frameCount % 3).bind(buf, this->pipelines.at(5));
    buf.dispatch(threadsX, threadsY, 1);

    if (!last)
        return;

    // seventh shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs1.at(0))
        .addR2W(this->tempImgs1.at(1))
        .build();

    this->pipelines.at(6).bind(buf);
    pass.descriptorSets.at(4).bind(buf, this->pipelines.at(6));
    buf.dispatch(threadsX, threadsY, 1);

    // eighth shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1.at(0))
        .addW2R(this->tempImgs1.at(1))
        .addR2W(this->tempImgs2)
        .build();
    this->pipelines.at(7).bind(buf);
    pass.descriptorSets.at(5).bind(buf, this->pipelines.at(7));
    buf.dispatch(threadsX, threadsY, 1);

    // ninth shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs1.at(0))
        .addR2W(this->tempImgs1.at(1))
        .build();

    this->pipelines.at(8).bind(buf);
    pass.descriptorSets.at(6).bind(buf, this->pipelines.at(8));
    buf.dispatch(threadsX, threadsY, 1);

    // tenth shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1.at(0))
        .addW2R(this->tempImgs1.at(1))
        .addR2W(this->outImg2)
        .build();

    this->pipelines.at(9).bind(buf);
    pass.descriptorSets.at(7).bind(buf, this->pipelines.at(9));
    buf.dispatch(threadsX, threadsY, 1);
}
