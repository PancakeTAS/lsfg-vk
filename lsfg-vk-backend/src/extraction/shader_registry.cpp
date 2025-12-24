/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "shader_registry.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/vulkan/shader.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

using namespace lsfgvk;
using namespace lsfgvk::backend;

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
            throw ls::error("unable to find shader with id: " + std::to_string(id));

        return it->second;
    }
    /// patch the generate shader
    void patchGenerateShader(std::vector<uint8_t>& data, bool hdr) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
        auto* _ptr = data.data();
        const std::span<uint32_t> words(
            reinterpret_cast<uint32_t*>(_ptr),
            data.size() / sizeof(uint32_t)
        );
#pragma clang diagnostic pop

        const uint16_t SpvOpCapability = 17;
        const uint16_t SpvOpTypeImage = 25;
        const uint32_t SpvCapabilityStorageImageWriteWithoutFormat = 56;
        const uint32_t SpvCapabilityShader = 1;
        const uint32_t SpvImageFormatRgba16f = 2;
        const uint32_t SpvImageFormatRgba8 = 4;

        for (size_t i = 5; i < words.size();) {
            const uint32_t& word = words[i];
            const uint16_t wc = (word >> 16);
            const uint16_t op = word & 0xFFFF;

            // remove write without format capability
            if (op == SpvOpCapability && wc >= 2) {
                uint32_t& cap = words[i + 1];
                if (cap == SpvCapabilityStorageImageWriteWithoutFormat)
                    cap = SpvCapabilityShader;
            }

            // patch format in image instructions
            if (op == SpvOpTypeImage && wc >= 9) {
                const uint32_t sampled = words[i + 7];
                if (sampled == 2)
                    words[i + 8] = hdr ? SpvImageFormatRgba16f : SpvImageFormatRgba8;
            }

            i += wc ? wc : 1;
        }
    }
}

ShaderRegistry backend::buildShaderRegistry(const vk::Vulkan& vk, bool fp16,
        const std::unordered_map<uint32_t, std::vector<uint8_t>>& resources) {
    // patch the generate shader
    std::vector<uint8_t> generate_data = getShaderSource(256, fp16, false, resources);
    std::vector<uint8_t> generate_data_hdr = generate_data;
    patchGenerateShader(generate_data, false);
    patchGenerateShader(generate_data_hdr, true);

    // load all other shaders
#define SHADER(id, p1, p2, p3, p4) \
    vk::Shader(vk, getShaderSource(id, fp16, PERF, resources), \
        p1, p2, p3, p4)

    return {
#define PERF false
        .mipmaps = SHADER(255, 1, 7, 1, 1),
        .generate = vk::Shader(vk, generate_data, 5, 1, 1, 2),
        .generate_hdr = vk::Shader(vk, generate_data_hdr, 5, 1, 1, 2),
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
