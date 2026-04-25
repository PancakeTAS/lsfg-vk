/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace lsfgvk::cli::debug {

    ///
    /// Options for the "debug" command
    ///
    struct Options {
        std::optional<std::string> dll;
        bool allow_fp16{false};
        int width{1920};
        int height{1080};

        float flow{1.0F};
        int multiplier{2};
        bool performance_mode{false};
        std::optional<std::string> gpu;

        std::filesystem::path path;
    };

    ///
    /// Run the "debug" command
    ///
    /// @param opts Command options
    /// @return Exit code
    ///
    int run(const Options& opts);

}
