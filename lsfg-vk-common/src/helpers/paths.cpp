/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/helpers/paths.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"

#include <cstdlib>
#include <filesystem>
#include <vector>

std::filesystem::path ls::findShaderDll() {
    const std::vector<std::filesystem::path> FRAGMENTS{{
        ".local/share/Steam/steamapps/common",
        ".steam/steam/steamapps/common",
        ".steam/debian-installation/steamapps/common",
        ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common",
        "snap/steam/common/.local/share/Steam/steamapps/common"
    }};

    // check XDG overridden location
    const char* xdgPath = std::getenv("XDG_DATA_HOME");
    if (xdgPath && *xdgPath != '\0') {
        auto base = std::filesystem::path(xdgPath);

        for (const auto& frag : FRAGMENTS) {
            auto full = base / frag / "Lossless Scaling" / "Lossless.dll";
            if (std::filesystem::exists(full))
                return full;
        }
    }

    // check home directory
    const char* homePath = std::getenv("HOME");
    if (homePath && *homePath != '\0') {
        auto base = std::filesystem::path(homePath);

        for (const auto& frag : FRAGMENTS) {
            auto full = base / frag / "Lossless Scaling" / "Lossless.dll";
            if (std::filesystem::exists(full))
                return full;
        }
    }

    // fallback to same directory
    auto local = std::filesystem::current_path() / "Lossless.dll";
    if (std::filesystem::exists(local))
        return local;

    throw ls::error("unable to locate Lossless.dll, please set the path in the configuration");
}
