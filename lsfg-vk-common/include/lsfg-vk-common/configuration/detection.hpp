/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "config.hpp"

#include <optional>
#include <string>
#include <utility>

namespace ls {

    /// identification data for a process
    struct Identification {
        /// optional override name
        std::optional<std::string> override;
        /// path to exe file
        std::string executable;
        /// path to exe file when running under wine
        std::optional<std::string> wine_executable;
        /// traditional process name (e.g. GameThread)
        std::string process_name;
    };

    /// enum describing which identification method was used
    enum class IdentType {
        OVERRIDE, // identified by override
        EXECUTABLE, // identified by executable path
        WINE_EXECUTABLE, // identified by wine executable path
        PROCESS_NAME // identified by process name
    };

    /// identify the current process
    Identification identify();

    /// find a profile for the current process
    /// @param config configuration to search in
    /// @param id identification data
    /// @return ident pair if found
    std::optional<std::pair<IdentType, GameConf>> findProfile(
        const ConfigFile& config, const Identification& id);
}
