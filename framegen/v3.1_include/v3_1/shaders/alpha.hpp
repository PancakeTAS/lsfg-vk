#pragma once

#include "core/commandbuffer.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/sampler.hpp"
#include "core/shadermodule.hpp"
#include "common/utils.hpp"

#include <array>
#include <cstdint>

namespace LSFG_3_1::Shaders {

    using namespace LSFG;

    ///
    /// Alpha shader.
    ///
    class Alpha {
    public:
        Alpha() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param inImg One mipmap level
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Alpha(Vulkan& vk, Core::Image inImg);

        ///
        /// Dispatch the shaderchain.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount);

        /// Get the output images
        [[nodiscard]] const auto& getOutImages() const { return this->outImgs; }

        /// Trivially copyable, moveable and destructible
        Alpha(const Alpha&) noexcept = default;
        Alpha& operator=(const Alpha&) noexcept = default;
        Alpha(Alpha&&) noexcept = default;
        Alpha& operator=(Alpha&&) noexcept = default;
        ~Alpha() = default;
    private:
        std::array<Core::ShaderModule, 4> shaderModules;
        std::array<Core::Pipeline, 4> pipelines;
        Core::Sampler sampler;
        std::array<Core::DescriptorSet, 3> descriptorSets;
        std::array<Core::DescriptorSet, 3> lastDescriptorSet;

        Core::Image inImg;
        std::array<Core::Image, 2> tempImgs1;
        std::array<Core::Image, 2> tempImgs2;
        std::array<Core::Image, 4> tempImgs3;
        std::array<std::array<Core::Image, 4>, 3> outImgs;
    };

}
