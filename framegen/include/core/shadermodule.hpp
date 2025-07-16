#pragma once

#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstddef>
#include <utility>
#include <vector>
#include <memory>

namespace LSFG::Core {

    ///
    /// C++ wrapper class for a Vulkan shader module.
    ///
    /// This class manages the lifetime of a Vulkan shader module.
    ///
    class ShaderModule {
    public:
        ShaderModule() noexcept = default;

        ///
        /// Create the shader module.
        ///
        /// @param device Vulkan device
        /// @param code SPIR-V bytecode for the shader.
        /// @param descriptorTypes Descriptor types used in the shader.
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        ShaderModule(const Core::Device& device, const std::vector<uint8_t>& code,
            const std::vector<std::pair<size_t, VkDescriptorType>>& descriptorTypes);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->shaderModule; }
        /// Get the descriptor set layout.
        [[nodiscard]] auto getLayout() const { return *this->descriptorSetLayout; }

        /// Trivially copyable, moveable and destructible
        ShaderModule(const ShaderModule&) noexcept = default;
        ShaderModule& operator=(const ShaderModule&) noexcept = default;
        ShaderModule(ShaderModule&&) noexcept = default;
        ShaderModule& operator=(ShaderModule&&) noexcept = default;
        ~ShaderModule() = default;
    private:
        std::shared_ptr<VkShaderModule> shaderModule;
        std::shared_ptr<VkDescriptorSetLayout> descriptorSetLayout;
    };

}
