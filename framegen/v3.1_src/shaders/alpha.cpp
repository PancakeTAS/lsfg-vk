#include "v3_1/shaders/alpha.hpp"
#include "common/utils.hpp"
#include "core/commandbuffer.hpp"
#include "core/image.hpp"

#include <vulkan/vulkan_core.h>

#include <utility>
#include <cstddef>
#include <cstdint>

using namespace LSFG_3_1::Shaders;

Alpha::Alpha(Vulkan& vk, Core::Image inImg) : inImg(std::move(inImg)) {
    // create resources
    this->shaderModules = {{
        vk.shaders.getShader(vk.device, "alpha[0]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "alpha[1]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "alpha[2]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "alpha[3]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } })
    }};
    this->pipelines = {{
        vk.shaders.getPipeline(vk.device, "alpha[0]"),
        vk.shaders.getPipeline(vk.device, "alpha[1]"),
        vk.shaders.getPipeline(vk.device, "alpha[2]"),
        vk.shaders.getPipeline(vk.device, "alpha[3]")
    }};
    this->sampler = vk.resources.getSampler(vk.device);
    for (size_t i = 0; i < 3; i++)
        this->descriptorSets.at(i) = Core::DescriptorSet(vk.device, vk.descriptorPool, this->shaderModules.at(i));
    for (size_t i = 0; i < 3; i++)
        this->lastDescriptorSet.at(i) = Core::DescriptorSet(vk.device, vk.descriptorPool, this->shaderModules.at(3));

    // create internal images/outputs
    const VkExtent2D extent = this->inImg.getExtent();
    const VkExtent2D halfExtent = {
        .width = (extent.width + 1) >> 1,
        .height = (extent.height + 1) >> 1
    };
    for (size_t i = 0; i < 2; i++) {
        this->tempImgs1.at(i) = Core::Image(vk.device, halfExtent);
        this->tempImgs2.at(i) = Core::Image(vk.device, halfExtent);
    }

    const VkExtent2D quarterExtent = {
        .width = (halfExtent.width + 1) >> 1,
        .height = (halfExtent.height + 1) >> 1
    };
    for (size_t i = 0; i < 4; i++) {
        this->tempImgs3.at(i) = Core::Image(vk.device, quarterExtent);
        for (size_t j = 0; j < 3; j++)
            this->outImgs.at(j).at(i) = Core::Image(vk.device, quarterExtent);
    }

    // hook up shaders
    this->descriptorSets.at(0).update(vk.device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->sampler)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
        .build();
    this->descriptorSets.at(1).update(vk.device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->sampler)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
        .build();
    this->descriptorSets.at(2).update(vk.device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->sampler)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs3)
        .build();
    for (size_t i = 0; i < 3; i++)
        this->lastDescriptorSet.at(i).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->sampler)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs3)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs.at(i))
            .build();
}

void Alpha::Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount) {
    // first pass
    const auto halfExtent = this->tempImgs1.at(0).getExtent();
    uint32_t threadsX = (halfExtent.width + 7) >> 3;
    uint32_t threadsY = (halfExtent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImg)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(0).bind(buf);
    this->descriptorSets.at(0).bind(buf, this->pipelines.at(0));
    buf.dispatch(threadsX, threadsY, 1);

    // second pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(1).bind(buf);
    this->descriptorSets.at(1).bind(buf, this->pipelines.at(1));
    buf.dispatch(threadsX, threadsY, 1);

    // third pass
    const auto quarterExtent = this->tempImgs3.at(0).getExtent();
    threadsX = (quarterExtent.width + 7) >> 3;
    threadsY = (quarterExtent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs3)
        .build();

    this->pipelines.at(2).bind(buf);
    this->descriptorSets.at(2).bind(buf, this->pipelines.at(2));
    buf.dispatch(threadsX, threadsY, 1);

    // fourth pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs3)
        .addR2W(this->outImgs.at(frameCount % 3))
        .build();

    this->pipelines.at(3).bind(buf);
    this->lastDescriptorSet.at(frameCount % 3).bind(buf, this->pipelines.at(3));
    buf.dispatch(threadsX, threadsY, 1);
}
