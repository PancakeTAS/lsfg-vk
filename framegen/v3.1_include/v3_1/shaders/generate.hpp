#pragma once

#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/sampler.hpp"
#include "core/shadermodule.hpp"
#include "common/utils.hpp"

#include <vulkan/vulkan_core.h>

#include <array>
#include <vector>
#include <cstdint>

namespace LSFG_3_1::Shaders {

    using namespace LSFG;

    ///
    /// Generate shader.
    ///
    class Generate {
    public:
        Generate() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param inImg1 Input image 1.
        /// @param inImg2 Input image 2.
        /// @param inImg3 Input image 3.
        /// @param inImg4 Input image 4.
        /// @param inImg5 Input image 5.
        /// @param fds File descriptors for the output images.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Generate(Vulkan& vk,
            Core::Image inImg1, Core::Image inImg2,
            Core::Image inImg3, Core::Image inImg4, Core::Image inImg5,
            const std::vector<int>& fds, VkFormat format);

        ///
        /// Dispatch the shaderchain.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount, uint64_t pass_idx);

        /// Trivially copyable, moveable and destructible
        Generate(const Generate&) noexcept = default;
        Generate& operator=(const Generate&) noexcept = default;
        Generate(Generate&&) noexcept = default;
        Generate& operator=(Generate&&) noexcept = default;
        ~Generate() = default;
    private:
        Core::ShaderModule shaderModule;
        Core::Pipeline pipeline;
        std::array<Core::Sampler, 2> samplers;
        struct GeneratePass {
            Core::Buffer buffer;
            std::array<Core::DescriptorSet, 2> descriptorSet;
        };
        std::vector<GeneratePass> passes;

        Core::Image inImg1, inImg2;
        Core::Image inImg3, inImg4, inImg5;
        std::vector<Core::Image> outImgs;
    };

}
