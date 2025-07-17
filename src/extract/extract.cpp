#include "extract/extract.hpp"
#include "config/config.hpp"

#include <pe-parse/parse.h>

#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace Extract;

const std::unordered_map<std::string, uint32_t> nameIdxTable = {{
    { "mipmaps", 255 },
    { "alpha[0]", 267 },
    { "alpha[1]", 268 },
    { "alpha[2]", 269 },
    { "alpha[3]", 270 },
    { "beta[0]", 275 },
    { "beta[1]", 276 },
    { "beta[2]", 277 },
    { "beta[3]", 278 },
    { "beta[4]", 279 },
    { "gamma[0]", 257 },
    { "gamma[1]", 259 },
    { "gamma[2]", 260 },
    { "gamma[3]", 261 },
    { "gamma[4]", 262 },
    { "delta[0]", 257 },
    { "delta[1]", 263 },
    { "delta[2]", 264 },
    { "delta[3]", 265 },
    { "delta[4]", 266 },
    { "delta[5]", 258 },
    { "delta[6]", 271 },
    { "delta[7]", 272 },
    { "delta[8]", 273 },
    { "delta[9]", 274 },
    { "generate", 256 },
    { "p_mipmaps", 255 },
    { "p_alpha[0]", 290 },
    { "p_alpha[1]", 291 },
    { "p_alpha[2]", 292 },
    { "p_alpha[3]", 293 },
    { "p_beta[0]", 298 },
    { "p_beta[1]", 299 },
    { "p_beta[2]", 300 },
    { "p_beta[3]", 301 },
    { "p_beta[4]", 302 },
    { "p_gamma[0]", 280 },
    { "p_gamma[1]", 282 },
    { "p_gamma[2]", 283 },
    { "p_gamma[3]", 284 },
    { "p_gamma[4]", 285 },
    { "p_delta[0]", 280 },
    { "p_delta[1]", 286 },
    { "p_delta[2]", 287 },
    { "p_delta[3]", 288 },
    { "p_delta[4]", 289 },
    { "p_delta[5]", 281 },
    { "p_delta[6]", 294 },
    { "p_delta[7]", 295 },
    { "p_delta[8]", 296 },
    { "p_delta[9]", 297 },
    { "p_generate", 256 },
}};

namespace {
    auto& shaders() {
        static std::unordered_map<uint32_t, std::vector<uint8_t>> shaderData;
        return shaderData;
    }

    int on_resource(void*, const peparse::resource& res) {
        if (res.type != peparse::RT_RCDATA || res.buf == nullptr || res.buf->bufLen <= 0)
            return 0;
        std::vector<uint8_t> resource_data(res.buf->bufLen);
        std::copy_n(res.buf->buf, res.buf->bufLen, resource_data.data());
        shaders()[res.name] = resource_data;
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
}

void Extract::extractShaders() {
    if (!shaders().empty())
        return;

    // parse the dll
    peparse::parsed_pe* dll = peparse::ParsePEFromFile(getDllPath().c_str());
    if (!dll)
        throw std::runtime_error("Unable to read Lossless.dll, is it installed?");
    peparse::IterRsrc(dll, on_resource, nullptr);
    peparse::DestructParsedPE(dll);
}

std::vector<uint8_t> Extract::getShader(const std::string& name) {
    if (shaders().empty())
        throw std::runtime_error("Shaders are not loaded.");

    auto hit = nameIdxTable.find(name);
    if (hit == nameIdxTable.end())
        throw std::runtime_error("Shader hash not found: " + name);

    auto sit = shaders().find(hit->second);
    if (sit == shaders().end())
        throw std::runtime_error("Shader not found: " + name);

    return sit->second;
}
