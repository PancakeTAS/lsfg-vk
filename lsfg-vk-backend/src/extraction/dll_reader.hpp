/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace lsfgvk::backend {

    /// extract all resources from a DLL file
    /// @param dll path to the DLL file
    /// @return map of resource IDs to their binary data
    /// @throws ls::error on various failure points
    std::unordered_map<uint32_t, std::vector<uint8_t>> extractResourcesFromDLL(
        const std::filesystem::path& dll);

}
