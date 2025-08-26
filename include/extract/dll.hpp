#pragma once

#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>

namespace DLL {

    ///
    /// Parse all resources from a DLL file.
    ///
    /// *Shouldn't* cause any segmentation faults.
    ///
    /// @param filename Path to the DLL file.
    /// @return A map of resource IDs to their binary data.
    ///
    /// @throws std::runtime_error on various failure points.
    ///
    std::unordered_map<uint32_t, std::vector<uint8_t>> parse_dll(const std::string& filename);

}
