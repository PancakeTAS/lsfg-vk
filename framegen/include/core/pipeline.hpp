#pragma once

#include "core/commandbuffer.hpp"
#include "core/shadermodule.hpp"
#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace LSFG::Core {

    ///
    /// C++ wrapper class for a Vulkan pipeline.
    ///
    /// This class manages the lifetime of a Vulkan pipeline.
    ///
    class Pipeline {
    public:
        Pipeline() noexcept = default;

        ///
        /// Create a compute pipeline.
        ///
        /// @param device Vulkan device
        /// @param shader Shader module to use for the pipeline.
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        Pipeline(const Core::Device& device, const ShaderModule& shader);

        ///
        /// Bind the pipeline to a command buffer.
        ///
        /// @param commandBuffer Command buffer to bind the pipeline to.
        ///
        void bind(const CommandBuffer& commandBuffer) const;

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->pipeline; }
        /// Get the pipeline layout.
        [[nodiscard]] auto getLayout() const { return *this->layout; }

        /// Trivially copyable, moveable and destructible
        Pipeline(const Pipeline&) noexcept = default;
        Pipeline& operator=(const Pipeline&) noexcept = default;
        Pipeline(Pipeline&&) noexcept = default;
        Pipeline& operator=(Pipeline&&) noexcept = default;
        ~Pipeline() = default;
    private:
        std::shared_ptr<VkPipeline> pipeline;
        std::shared_ptr<VkPipelineLayout> layout;
    };

}
