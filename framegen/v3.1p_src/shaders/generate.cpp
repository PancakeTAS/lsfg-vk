#include "v3_1p/shaders/generate.hpp"
#include "common/utils.hpp"
#include "core/commandbuffer.hpp"
#include "core/image.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>

using namespace LSFG_3_1P::Shaders;

Generate::Generate(Vulkan& vk,
    Core::Image inImg1, Core::Image inImg2,
    Core::Image inImg3, Core::Image inImg4, Core::Image inImg5,
    const std::vector<int>& fds, VkFormat format)
        : inImg1(std::move(inImg1)), inImg2(std::move(inImg2)),
          inImg3(std::move(inImg3)), inImg4(std::move(inImg4)),
          inImg5(std::move(inImg5)) {
    // create resources
    this->shaderModule = vk.shaders.getShader(vk.device, "p_generate",
        { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
          { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
          { 5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } });
    this->pipeline = vk.shaders.getPipeline(vk.device, "p_generate");
    this->samplers.at(0) = vk.resources.getSampler(vk.device);
    this->samplers.at(1) = vk.resources.getSampler(vk.device,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_COMPARE_OP_ALWAYS);

    // create internal images/outputs
    const VkExtent2D extent = this->inImg1.getExtent();
    for (size_t i = 0; i < vk.generationCount; i++)
        this->outImgs.emplace_back(vk.device, extent, format,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, fds.empty() ? -1 : fds.at(i));

    // hook up shaders
    for (size_t i = 0; i < vk.generationCount; i++) {
        auto& pass = this->passes.emplace_back();
        pass.buffer = vk.resources.getBuffer(vk.device,
            static_cast<float>(i + 1) / static_cast<float>(vk.generationCount + 1));
        for (size_t j = 0; j < 2; j++) {
            pass.descriptorSet.at(j) = Core::DescriptorSet(vk.device, vk.descriptorPool,
                this->shaderModule);
            pass.descriptorSet.at(j).update(vk.device)
                .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pass.buffer)
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, j == 0 ? this->inImg2 : this->inImg1)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, j == 0 ? this->inImg1 : this->inImg2)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg3)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg4)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg5)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs.at(i))
                .build();
        }
    }
}

void Generate::Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount, uint64_t pass_idx) {
    auto& pass = this->passes.at(pass_idx);

    // first pass
    const auto extent = this->inImg1.getExtent();
    const uint32_t threadsX = (extent.width + 15) >> 4;
    const uint32_t threadsY = (extent.height + 15) >> 4;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImg1)
        .addW2R(this->inImg2)
        .addW2R(this->inImg3)
        .addW2R(this->inImg4)
        .addW2R(this->inImg5)
        .addR2W(this->outImgs.at(pass_idx))
        .build();

    this->pipeline.bind(buf);
    pass.descriptorSet.at(frameCount % 2).bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
