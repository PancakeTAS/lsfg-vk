/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "library.hpp"
#include "library/dll.hpp"
#include "utility/vkhelper.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

/// All base shaders in the library.
const std::array<std::pair<std::string_view, uint32_t>, 3> BASE_LIBRARY{{
    { "mipmaps", 0 },
    { "generate_8bit", 1 },
    { "generate_16bit", 2 },
}};

/// All non-base shaders in the library.
const std::array<std::pair<std::string_view, uint32_t>, 24> LIBRARY{{
    { "alpha0", 13 },
    { "alpha1", 14 },
    { "alpha2", 15 },
    { "alpha3", 16 },
    { "beta0", 22 },
    { "beta1", 23 },
    { "beta2", 24 },
    { "beta3", 25 },
    { "beta4", 26 },
    { "gamma0", 3 },
    { "gamma1", 4 },
    { "gamma2", 5 },
    { "gamma3", 6 },
    { "gamma4", 7 },
    { "delta0", 8 },
    { "delta1", 9 },
    { "delta2", 10 },
    { "delta3", 11 },
    { "delta4", 12 },
    { "epsilon0", 17 },
    { "epsilon1", 18 },
    { "epsilon2", 19 },
    { "epsilon3", 20 },
    { "epsilon4", 21 }
}};

using namespace lsfgvk::library;

ShaderLibrary::ShaderLibrary(
    const vk::detail::DispatchLoaderDynamic& dld,
    const vk::Device& device,
    bool halfPrecision,
    const std::filesystem::path& dll
) {
    if (!std::filesystem::exists(dll)) {
        throw std::runtime_error("The specified shader DLL does not exist");
    }
    // Create shader modules for each shader in the library
    const auto resources = priv::parseDll(dll);
    for (const auto& [name, idx] : BASE_LIBRARY) {
        const uint32_t rid{idx};

        const auto& it = resources.find(rid == 0 ? 2147488584U : rid);
        if (it == resources.end())
            throw std::runtime_error(
                "Unable to find base shader '" + std::string(name) + "' in DLL"
            );

        this->m_baseShaders[name] = vkhelper::createShaderModule(dld, device, it->second);
    }

    for (const auto& [name, idx] : LIBRARY) {
        const std::pair<uint32_t, uint32_t> rid{
            idx + (halfPrecision ? 48 : 0),
            idx + (halfPrecision ? 48 : 0) + 24
        };

        const auto& qit{resources.find(rid.first)};
        const auto& pit{resources.find(rid.second)};
        if (qit == resources.end() || pit == resources.end())
            throw std::runtime_error(
                "Unable to find shader '" + std::string(name) + "' in DLL"
            );

        this->m_qualityShaders[name] = vkhelper::createShaderModule(dld, device, qit->second);
        this->m_performanceShaders[name] = vkhelper::createShaderModule(dld, device, pit->second);
    }
}
