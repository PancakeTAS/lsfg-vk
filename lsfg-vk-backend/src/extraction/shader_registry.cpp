#include "shader_registry.hpp"
#include "lsfg-vk-common/vulkan/shader.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace extr;

namespace {
    /// get the source code for a shader
    const std::vector<uint8_t>& getShaderSource(uint32_t id, bool fp16, bool perf,
            const std::unordered_map<uint32_t, std::vector<uint8_t>>& resources) {
        const size_t BASE_OFFSET = 49;
        const size_t OFFSET_PERF = 23;
        const size_t OFFSET_FP16 = 49;

        auto it = resources.find(BASE_OFFSET + id +
            (perf ? OFFSET_PERF : 0) +
            (fp16 ? OFFSET_FP16 : 0));
        if (it == resources.end())
            throw std::runtime_error("unable to find shader with id: " + std::to_string(id));

        return it->second;
    }
}

ShaderRegistry extr::buildShaderRegistry(const vk::Vulkan& vk, bool fp16,
        const std::unordered_map<uint32_t, std::vector<uint8_t>>& resources) {
#define SHADER(id, p1, p2, p3, p4) \
    vk::Shader(vk, getShaderSource(id, fp16, PERF, resources), \
        p1, p2, p3, p4)

    return {
#define PERF false
        .mipmaps = SHADER(255, 1, 7, 1, 1),
        .generate = SHADER(256, 5, 1, 1, 2),
        .quality = {
            .alpha = {
                SHADER(267, 1, 2, 0, 1),
                SHADER(268, 2, 2, 0, 1),
                SHADER(269, 2, 4, 0, 1),
                SHADER(270, 4, 4, 0, 1)
            },
            .beta = {
                SHADER(275, 12, 2, 0, 1),
                SHADER(276, 2, 2, 0, 1),
                SHADER(277, 2, 2, 0, 1),
                SHADER(278, 2, 2, 0, 1),
                SHADER(279, 2, 6, 1, 1)
            },
            .gamma = {
                SHADER(257, 9, 3, 1, 2),
                SHADER(259, 3, 4, 0, 1),
                SHADER(260, 4, 4, 0, 1),
                SHADER(261, 4, 4, 0, 1),
                SHADER(262, 6, 1, 1, 2)
            },
            .delta = {
                SHADER(257, 9, 3, 1, 2),
                SHADER(263, 3, 4, 0, 1),
                SHADER(264, 4, 4, 0, 1),
                SHADER(265, 4, 4, 0, 1),
                SHADER(266, 6, 1, 1, 2),
                SHADER(258, 10, 2, 1, 2),
                SHADER(271, 2, 2, 0, 1),
                SHADER(272, 2, 2, 0, 1),
                SHADER(273, 2, 2, 0, 1),
                SHADER(274, 3, 1, 1, 2)
            }
        },
#undef PERF
#define PERF true
        .performance = {
            .alpha = {
                SHADER(267, 1, 1, 0, 1),
                SHADER(268, 1, 1, 0, 1),
                SHADER(269, 1, 2, 0, 1),
                SHADER(270, 2, 2, 0, 1)
            },
            .beta = {
                SHADER(275, 6, 2, 0, 1),
                SHADER(276, 2, 2, 0, 1),
                SHADER(277, 2, 2, 0, 1),
                SHADER(278, 2, 2, 0, 1),
                SHADER(279, 2, 6, 1, 1)
            },
            .gamma = {
                SHADER(257, 5, 3, 1, 2),
                SHADER(259, 3, 2, 0, 1),
                SHADER(260, 2, 2, 0, 1),
                SHADER(261, 2, 2, 0, 1),
                SHADER(262, 4, 1, 1, 2)
            },
            .delta = {
                SHADER(257, 5, 3, 1, 2),
                SHADER(263, 3, 2, 0, 1),
                SHADER(264, 2, 2, 0, 1),
                SHADER(265, 2, 2, 0, 1),
                SHADER(266, 4, 1, 1, 2),
                SHADER(258, 6, 1, 1, 2),
                SHADER(271, 1, 1, 0, 1),
                SHADER(272, 1, 1, 0, 1),
                SHADER(273, 1, 1, 0, 1),
                SHADER(274, 2, 1, 1, 2)
            }
        },
#undef PERF
        .is_fp16 = fp16
    };

#undef SHADER
}
