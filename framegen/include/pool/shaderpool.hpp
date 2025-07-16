#pragma once

#include "core/device.hpp"
#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace LSFG::Pool {

    ///
    /// Shader pool for each Vulkan device.
    ///
    class ShaderPool {
    public:
        ShaderPool() noexcept = default;

        ///
        /// Create the shader pool.
        ///
        /// @param source Function to retrieve shader source code by name.
        ///
        /// @throws std::runtime_error if the shader pool cannot be created.
        ///
        ShaderPool(const std::function<std::vector<uint8_t>(const std::string&)>& source)
            : source(source) {}

        ///
        /// Retrieve a shader module by name or create it.
        ///
        /// @param name Name of the shader module
        /// @param types Descriptor types for the shader module
        /// @return Shader module
        ///
        /// @throws LSFG::vulkan_error if the shader module cannot be created.
        ///
        Core::ShaderModule getShader(
            const Core::Device& device, const std::string& name,
            const std::vector<std::pair<size_t, VkDescriptorType>>& types);

        ///
        /// Retrieve a pipeline shader module by name or create it.
        ///
        /// @param name Name of the shader module
        /// @return Pipeline shader module or empty
        ///
        /// @throws LSFG::vulkan_error if the shader module cannot be created.
        ///
        Core::Pipeline getPipeline(
            const Core::Device& device, const std::string& name);
    private:
        std::function<std::vector<uint8_t>(const std::string&)> source;
        std::unordered_map<std::string, Core::ShaderModule> shaders;
        std::unordered_map<std::string, Core::Pipeline> pipelines;
    };

}
