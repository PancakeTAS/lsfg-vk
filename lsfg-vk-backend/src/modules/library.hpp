/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "utility/vkhelper.hpp"

#include <filesystem>
#include <string_view>
#include <unordered_map>

namespace lsfgvk::library {

    ///
    /// The lsfg-vk shader library
    ///
    class ShaderLibrary {
    public:
        ///
        /// Create the shader library
        ///
        /// @param dld Vulkan dynamic dispatch loader
        /// @param device Vulkan device
        /// @param halfPrecision Whether to load the half-precision shader variants
        /// @param dll Path to the shader DLL file
        /// @throws std::runtime_error on failure
        ///
        explicit ShaderLibrary(
            const vk::detail::DispatchLoaderDynamic& dld,
            const vk::Device& device,
            bool halfPrecision,
            const std::filesystem::path& dll
        );

        ///
        /// Get a base shader by name
        ///
        /// @param name Shader name
        /// @return A reference to the shader
        /// @throws std::out_of_range if the shader is not found
        ///
        [[nodiscard]] const auto& baseShader(std::string_view name) const {
            return this->m_baseShaders.at(name);
        }

        ///
        /// Get a shader by name
        ///
        /// @param name Shader name
        /// @param perf Whether to get the performance variant of the shader
        /// @return A reference to the shader
        /// @throws std::out_of_range if the shader is not found
        ///
        [[nodiscard]] const auto& shader(std::string_view name, bool perf) const {
            auto it{this->m_baseShaders.find(name)};
            if (it != this->m_baseShaders.end())
                return it->second;

            return perf ? this->m_performanceShaders.at(name) : this->m_qualityShaders.at(name);
        }

    private:
        std::unordered_map<std::string_view, vk::UniqueShaderModule> m_baseShaders;
        std::unordered_map<std::string_view, vk::UniqueShaderModule> m_qualityShaders;
        std::unordered_map<std::string_view, vk::UniqueShaderModule> m_performanceShaders;
    };

}
