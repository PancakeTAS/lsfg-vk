/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "library.hpp"
#include "pipeline/signature.hpp"
#include "utility/vkhelper.hpp"

#include <cstdint>

namespace lsfgvk::pipeline {

    // TODO: Improve API design

    /// Struct for the uniform buffer
    struct UniformBuffer {
        float timestamp;
        uint32_t iteration;
        uint32_t advancedColorKind;
        uint32_t hdrSupport;
        float resolutionInvScale;
        float uiThreshold;
    };

    /// Struct for push constants
    struct PushConstants {
        uint32_t specialFlag;
        uint32_t subiteration;
    };

    ///
    /// Vulkan pipeline created from a signature
    ///
    class Pipeline {
    public:
        ///
        /// Create a new pipeline
        ///
        /// @param dld Vulkan dispatch loader
        /// @param dev Vulkan device
        /// @param physdev Vulkan physical device
        /// @param queue Vulkan compute queue
        /// @param queueFamilyIndex Compute queue family index
        /// @param library Shader library
        /// @param signature Pipeline signature
        /// @param extent Base extent
        /// @param flow Flow scale
        /// @param perf Performance mode
        /// @param hdr HDR variant
        /// @throws std::runtime_error on failure
        ///
        explicit Pipeline(
            const vk::detail::DispatchLoaderDynamic& dld,
            const vk::Device& dev,
            const vk::PhysicalDevice& physdev,
            const vk::Queue& queue,
            uint32_t queueFamilyIndex,
            const library::ShaderLibrary& library,
            const PipelineSignature& signature,
            vk::Extent2D extent,
            float flow,
            bool perf,
            bool hdr
        );

    private:
        /// Vulkan descriptor set & pipeline layout
        struct Layout {
            vk::UniqueDescriptorSetLayout layout;
            vk::UniquePipelineLayout pipelineLayout;
        };
        Layout m_layout;
    };

}
