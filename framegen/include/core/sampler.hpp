#pragma once

#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace LSFG::Core {

    ///
    /// C++ wrapper class for a Vulkan sampler.
    ///
    /// This class manages the lifetime of a Vulkan sampler.
    ///
    class Sampler {
    public:
        Sampler() noexcept = default;

        ///
        /// Create the sampler.
        ///
        /// @param device Vulkan device
        /// @param mode Address mode for the sampler.
        /// @param compare Compare operation for the sampler.
        /// @param isWhite Whether the border color is white.
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        Sampler(const Core::Device& device,
            VkSamplerAddressMode mode,
            VkCompareOp compare,
            bool isWhite);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->sampler; }

        /// Trivially copyable, moveable and destructible
        Sampler(const Sampler&) noexcept = default;
        Sampler& operator=(const Sampler&) noexcept = default;
        Sampler(Sampler&&) noexcept = default;
        Sampler& operator=(Sampler&&) noexcept = default;
        ~Sampler() = default;
    private:
        std::shared_ptr<VkSampler> sampler;
    };

}
