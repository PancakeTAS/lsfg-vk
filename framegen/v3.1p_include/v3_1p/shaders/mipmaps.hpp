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
    /// Mipmaps shader.
    ///
    class Mipmaps {
    public:
        Mipmaps() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param inImg_0 The next frame (when fc % 2 == 0)
        /// @param inImg_1 The next frame (when fc % 2 == 1)
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Mipmaps(Vulkan& vk, Core::Image inImg_0, Core::Image inImg_1);

        ///
        /// Dispatch the shaderchain.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount);

        /// Get the output images.
        [[nodiscard]] const auto& getOutImages() const { return this->outImgs; }

        /// Trivially copyable, moveable and destructible
        Mipmaps(const Mipmaps&) noexcept = default;
        Mipmaps& operator=(const Mipmaps&) noexcept = default;
        Mipmaps(Mipmaps&&) noexcept = default;
        Mipmaps& operator=(Mipmaps&&) noexcept = default;
        ~Mipmaps() = default;
    private:
        Core::ShaderModule shaderModule;
        Core::Pipeline pipeline;
        Core::Buffer buffer;
        Core::Sampler sampler;
        std::array<Core::DescriptorSet, 2> descriptorSets;

        Core::Image inImg_0, inImg_1;
        std::array<Core::Image, 7> outImgs;
    };

}
