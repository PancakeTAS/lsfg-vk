#include "extract/extract.hpp"
#include "config/config.hpp"
#include "extract/dll.hpp"

#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <array>

using namespace Extract;

const uint32_t NO = 49; // native offset
const uint32_t PO = NO + 23; // performance+native offset
const uint32_t FP = 49; // fp32 offset
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
    { "generate", 256 + NO },
    { "p_alpha[0]", 267 + PO },
    { "p_alpha[1]", 268 + PO },
    { "p_alpha[2]", 269 + PO },
    { "p_alpha[3]", 270 + PO },
    { "p_beta[0]",  275 + PO },
    { "p_beta[1]",  276 + PO },
    { "p_beta[2]",  277 + PO },
    { "p_beta[3]",  278 + PO },
    { "p_beta[4]",  279 + PO },
    { "p_gamma[0]", 257 + PO },
    { "p_gamma[1]", 259 + PO },
    { "p_gamma[2]", 260 + PO },
    { "p_gamma[3]", 261 + PO },
    { "p_gamma[4]", 262 + PO },
    { "p_delta[0]", 257 + PO },
    { "p_delta[1]", 263 + PO },
    { "p_delta[2]", 264 + PO },
    { "p_delta[3]", 265 + PO },
    { "p_delta[4]", 266 + PO },
    { "p_delta[5]", 258 + PO },
    { "p_delta[6]", 271 + PO },
    { "p_delta[7]", 272 + PO },
    { "p_delta[8]", 273 + PO },
    { "p_delta[9]", 274 + PO },
}};

namespace {
    auto& shaders() {
        static std::unordered_map<uint32_t, std::array<std::vector<uint8_t>, 2>> shaderData;
        return shaderData;
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
        std::string dllPath = Config::globalConf.dll;
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
}

void Extract::extractShaders() {
    if (!shaders().empty())
        return;

    // parse the dll
    const auto resources = DLL::parse_dll(getDllPath());
    std::cerr << "lsfg-vk: Extracted " << resources.size() << " resources from dll.\n";

    // ensure all shaders are present
    for (const auto& [name, idx] : nameIdxTable) {
        auto fp16 = resources.find(idx);
        if (fp16 == resources.end())
            throw std::runtime_error("Shader not found: " + name + " (FP16).\n- Is Lossless Scaling up to date?");
        auto fp32 = resources.find(idx + FP);
        if (fp32 == resources.end())
            throw std::runtime_error("Shader not found: " + name + " (FP32).\n- Is Lossless Scaling up to date?");

        shaders().emplace(idx, std::array<std::vector<uint8_t>, 2>{
            fp32->second,
            fp16->second
        });
    }
}

std::vector<uint8_t> Extract::getShader(const std::string& name, bool fp16) {
    if (shaders().empty())
        throw std::runtime_error("Shaders are not loaded.");

    auto hit = nameIdxTable.find(name);
    if (hit == nameIdxTable.end())
        throw std::runtime_error("Shader hash not found: " + name);

    auto sit = shaders().find(hit->second);
    if (sit == shaders().end())
        throw std::runtime_error("Shader not found: " + name);
    return fp16 ? sit->second.at(1) : sit->second.at(0);
}
