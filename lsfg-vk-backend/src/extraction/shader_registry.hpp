#pragma once

#include "lsfg-vk-common/vulkan/shader.hpp"

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace extr {

    /// shader collection struct
    struct Shaders {
        std::array<vk::Shader, 4> alpha;
        std::array<vk::Shader, 5> beta;
        std::array<vk::Shader, 5> gamma;
        std::array<vk::Shader, 10> delta;
    };

    /// shader registry struct
    struct ShaderRegistry {
        vk::Shader mipmaps;
        vk::Shader generate;
        Shaders quality;
        Shaders performance;

        bool is_fp16; //!< whether the fp16 shader variants were loaded
    };

    /// build a shader registry from resources
    /// @param vk Vulkan instance
    /// @param fp16 whether to load fp16 variants
    /// @param resources map of resource IDs to their binary data
    /// @return constructed shader registry
    /// @throws std::runtime_error if shaders are missing
    /// @throws vk::vulkan_error on Vulkan errors
    ShaderRegistry buildShaderRegistry(const vk::Vulkan& vk, bool fp16,
        const std::unordered_map<uint32_t, std::vector<uint8_t>>& resources);

}
