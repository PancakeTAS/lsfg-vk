/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace lsfgvk::library::priv {

    ///
    /// Parse all resources from a DLL file
    ///
    /// @param dll File path
    /// @returns Map of resource ID to data
    /// @throws std::runtime_error if the file is invalid or cannot be read
    ///
    std::unordered_map<uint32_t, std::vector<uint32_t>> parseDll(
        const std::filesystem::path& dll
    );

}
