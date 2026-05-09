/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#define LSFGVK_PRIV
#include "lsfg-vk/lsfgvk.hpp" // IWYU pragma: export

#include "modules/pipeline.hpp"
#include "modules/library.hpp"
#include "utility/vkhelper.hpp"

#include <cstdint>
#include <functional>
#include <utility>

namespace lsfgvk::priv {


    /// Internal state of lsfg-vk
    struct Instance {
        /// Vulkan context
        struct Vulkan {
            /// Vulkan dispatch loader
            std::unique_ptr<vk::detail::DispatchLoaderDynamic> dld;
            /// Vulkan instance (1.2)
            vk::UniqueInstance instance;
            /// Vulkan physical device
            vk::PhysicalDevice physdev;
            /// Vulkan device with synchronization2 (extension), external memory & semaphore
            /// fd (extension) and timeline semaphores (core) enabled
            vk::UniqueDevice device;
            /// Compute queue
            vk::Queue queue;
            /// Compute queue family index
            uint32_t qfi;
            /// Whether fp16 is enabled and supported (shaderFloat16 is enabled)
            bool fp16;
        } vk;
        /// Shader library
        library::ShaderLibrary shaderLibrary;
    };

    /// Internal context for frame generation
    struct Context {
        /// Parent instance
        std::reference_wrapper<Instance> instance;
        /// Pipeline instance
        pipeline::Pipeline pipeline;
        /// Shared synchronization semaphores
        std::pair<vk::UniqueSemaphore, uint64_t> syncSemaphore;
        /// Internal synchronization semaphores
        std::pair<vk::UniqueSemaphore, uint64_t> internalSemaphores;
        /// Frames-in-flight fence
        vk::UniqueFence fence;
        /// Is first iteration
        bool firstIteration{true};
    };

}
