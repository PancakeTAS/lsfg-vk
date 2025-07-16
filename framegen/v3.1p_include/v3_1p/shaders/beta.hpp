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

namespace LSFG_3_1P::Shaders {

    using namespace LSFG;

    ///
    /// Beta shader.
    ///
    class Beta {
    public:
        Beta() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param inImgs Three sets of two RGBA images, corresponding to a frame count % 3.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Beta(Vulkan& vk, std::array<std::array<Core::Image, 2>, 3> inImgs);

        ///
        /// Dispatch the shaderchain.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount);

        /// Get the output images
        [[nodiscard]] const auto& getOutImages() const { return this->outImgs; }

        /// Trivially copyable, moveable and destructible
        Beta(const Beta&) noexcept = default;
        Beta& operator=(const Beta&) noexcept = default;
        Beta(Beta&&) noexcept = default;
        Beta& operator=(Beta&&) noexcept = default;
        ~Beta() = default;
    private:
        std::array<Core::ShaderModule, 5> shaderModules;
        std::array<Core::Pipeline, 5> pipelines;
        std::array<Core::Sampler, 2> samplers;
        Core::Buffer buffer;
        std::array<Core::DescriptorSet, 3> firstDescriptorSet;
        std::array<Core::DescriptorSet, 4> descriptorSets;

        std::array<std::array<Core::Image, 2>, 3> inImgs;
        std::array<Core::Image, 2> tempImgs1;
        std::array<Core::Image, 2> tempImgs2;
        std::array<Core::Image, 6> outImgs;
    };

}
