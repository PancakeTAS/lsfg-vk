#pragma once

#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/sampler.hpp"
#include "core/shadermodule.hpp"
#include "common/utils.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace LSFG_3_1::Shaders {

    using namespace LSFG;

    ///
    /// Gamma shader.
    ///
    class Gamma {
    public:
        Gamma() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param inImgs1 Three sets of four RGBA images, corresponding to a frame count % 3.
        /// @param inImg2 Second Input image
        /// @param optImg Optional image for non-first passes.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Gamma(Vulkan& vk, std::array<std::array<Core::Image, 4>, 3> inImgs1,
            Core::Image inImg2, std::optional<Core::Image> optImg);

        ///
        /// Dispatch the shaderchain.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount, uint64_t pass_idx);

        /// Get the output image
        [[nodiscard]] const auto& getOutImage() const { return this->outImg; }

        /// Trivially copyable, moveable and destructible
        Gamma(const Gamma&) noexcept = default;
        Gamma& operator=(const Gamma&) noexcept = default;
        Gamma(Gamma&&) noexcept = default;
        Gamma& operator=(Gamma&&) noexcept = default;
        ~Gamma() = default;
    private:
        std::array<Core::ShaderModule, 5> shaderModules;
        std::array<Core::Pipeline, 5> pipelines;
        std::array<Core::Sampler, 3> samplers;
        struct GammaPass {
            Core::Buffer buffer;
            std::array<Core::DescriptorSet, 3> firstDescriptorSet;
            std::array<Core::DescriptorSet, 4> descriptorSets;
        };
        std::vector<GammaPass> passes;

        std::array<std::array<Core::Image, 4>, 3> inImgs1;
        Core::Image inImg2;
        std::optional<Core::Image> optImg;
        std::array<Core::Image, 4> tempImgs1;
        std::array<Core::Image, 4> tempImgs2;
        Core::Image outImg;
    };

}
