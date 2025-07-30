#include "extract/extract.hpp"
#include "config/config.hpp"

#include <spirv-tools/optimizer.hpp>
#include <spirv-tools/libspirv.h>
#include <pe-parse/parse.h>

#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <array>

using namespace Extract;

const uint32_t NO = 49; // native offset
const std::unordered_map<std::string, uint32_t> nameIdxTable = {{
    { "mipmaps",  255 + NO },
    { "alpha[0]", 267 + NO },
    { "alpha[1]", 268 + NO },
    { "alpha[2]", 269 + NO },
    { "alpha[3]", 270 + NO },
    { "beta[0]",  275 + NO },
    { "beta[1]",  276 + NO },
    { "beta[2]",  277 + NO },
    { "beta[3]",  278 + NO },
    { "beta[4]",  279 + NO },
    { "gamma[0]", 257 + NO },
    { "gamma[1]", 259 + NO },
    { "gamma[2]", 260 + NO },
    { "gamma[3]", 261 + NO },
    { "gamma[4]", 262 + NO },
    { "delta[0]", 257 + NO },
    { "delta[1]", 263 + NO },
    { "delta[2]", 264 + NO },
    { "delta[3]", 265 + NO },
    { "delta[4]", 266 + NO },
    { "delta[5]", 258 + NO },
    { "delta[6]", 271 + NO },
    { "delta[7]", 272 + NO },
    { "delta[8]", 273 + NO },
    { "delta[9]", 274 + NO },
    { "generate", 256 + NO }
}};

namespace {
    auto& pshaders() {
        static std::unordered_map<uint32_t, std::array<std::vector<uint8_t>, 2>> shaderData;
        return shaderData;
    }

    int on_resource(void* ptr, const peparse::resource& res) {
        if (res.type != peparse::RT_RCDATA || res.buf == nullptr || res.buf->bufLen <= 0)
            return 0;
        std::vector<uint8_t> resource_data(res.buf->bufLen);
        std::copy_n(res.buf->buf, res.buf->bufLen, resource_data.data());

        auto* shaders = reinterpret_cast<std::unordered_map<uint32_t, std::vector<uint8_t>>*>(ptr);
        shaders->emplace(res.name, std::move(resource_data));
        return 0;
    }

    const std::vector<std::filesystem::path> PATHS{{
        ".local/share/Steam/steamapps/common",
        ".steam/steam/steamapps/common",
        ".steam/debian-installation/steamapps/common",
        ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common",
        "snap/steam/common/.local/share/Steam/steamapps/common"
    }};

    std::string getDllPath() {
        // overriden path
        std::string dllPath = Config::activeConf.dll;
        if (!dllPath.empty())
            return dllPath;
        // home based paths
        const char* home = getenv("HOME");
        const std::string homeStr = home ? home : "";
        for (const auto& base : PATHS) {
            const std::filesystem::path path =
                std::filesystem::path(homeStr) / base / "Lossless Scaling" / "Lossless.dll";
            if (std::filesystem::exists(path))
                return path.string();
        }
        // xdg home
        const char* dataDir = getenv("XDG_DATA_HOME");
        if (dataDir && *dataDir != '\0')
            return std::string(dataDir) + "/Steam/steamapps/common/Lossless Scaling/Lossless.dll";
        // final fallback
        return "Lossless.dll";
    }

    std::array<std::vector<uint8_t>, 2> fixShaders(const std::vector<uint8_t>& spirv) {
        std::vector<uint32_t> shader(spirv.size() / 4);
        std::copy_n(spirv.data(), spirv.size(), reinterpret_cast<uint8_t*>(shader.data()));

        // patch bindings
        std::vector<size_t> samplerOffsets{};
        std::vector<size_t> sampledImageOffsets{};
        std::vector<size_t> storageImageOffsets{};
        std::vector<size_t> uniformBufferOffsets{};

        uint32_t prevIdx{ 0 };
        uint32_t type{ 0 };

        size_t i{ 5 };
        while (i < shader.size()) {
            const uint32_t word = shader[i];
            const uint16_t op = word & 0xFFFF;
            const uint16_t len = word >> 16;
            if (op == 71 /*spv::OpDecorate*/) {
                const uint32_t decoration = shader[i + 2];
                if (decoration == 33 /*spv::DecorationBinding*/) {
                    const uint32_t idx = shader[i + 3];
                    if (idx <= prevIdx)
                        type++;
                    prevIdx = idx;

                    switch (type) {
                        case 1:
                            samplerOffsets.emplace_back(i + 3);
                            break;
                        case 2:
                            sampledImageOffsets.emplace_back(i + 3);
                            break;
                        case 3:
                            storageImageOffsets.emplace_back(i + 3);
                            break;
                        case 4:
                            uniformBufferOffsets.emplace_back(i + 3);
                            break;
                        default:
                            break;
                    }
                }
            }

            if (op == 54 /*spv::OpFunction*/)
                break;

            i += len ? len : 1;
        }

        uint32_t binding{ 0 };
        for (const auto& idx : uniformBufferOffsets)
            shader[idx] = binding++;
        for (const auto& idx : samplerOffsets)
            shader[idx] = binding++;
        for (const auto& idx : sampledImageOffsets)
            shader[idx] = binding++;
        for (const auto& idx : storageImageOffsets)
            shader[idx] = binding++;

        std::vector<uint8_t> result_fp32(shader.size() * sizeof(uint32_t));
        std::copy_n(reinterpret_cast<uint8_t*>(shader.data()),
            result_fp32.size(), result_fp32.data());

        spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_3);
        optimizer.RegisterPass(spvtools::CreateConvertRelaxedToHalfPass());
        optimizer.Run(shader.data(), shader.size() * sizeof(uint32_t), &shader);

        std::vector<uint8_t> result_fp16(shader.size() * sizeof(uint32_t));
        std::copy_n(reinterpret_cast<uint8_t*>(shader.data()),
            result_fp16.size(), result_fp16.data());
        return { std::move(result_fp32), std::move(result_fp16) };
    }
}

void Extract::extractShaders() {
    if (!pshaders().empty())
        return;

    std::unordered_map<uint32_t, std::vector<uint8_t>> shaders{};

    // parse the dll
    peparse::parsed_pe* dll = peparse::ParsePEFromFile(getDllPath().c_str());
    if (!dll)
        throw std::runtime_error("Unable to read Lossless.dll, is it installed?");
    peparse::IterRsrc(dll, on_resource, reinterpret_cast<void*>(&shaders));
    peparse::DestructParsedPE(dll);

    // ensure all shaders are present
    for (const auto& [name, idx] : nameIdxTable)
        if (shaders.find(idx) == shaders.end())
            throw std::runtime_error("Shader not found: " + name + ".\n- Is Lossless Scaling up to date?");

    // fix shader bytecode
    for (auto& [idx, data] : shaders)
        pshaders()[idx] = fixShaders(data);
}

std::vector<uint8_t> Extract::getShader(const std::string& name, bool fp16) {
    if (pshaders().empty())
        throw std::runtime_error("Shaders are not loaded.");

    auto hit = nameIdxTable.find(name);
    if (hit == nameIdxTable.end())
        throw std::runtime_error("Shader hash not found: " + name);

    auto sit = pshaders().find(hit->second);
    if (sit == pshaders().end())
        throw std::runtime_error("Shader not found: " + name);
    return fp16 ? sit->second.at(1) : sit->second.at(0);
}
