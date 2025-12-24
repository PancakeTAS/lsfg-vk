/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace lsfgvk::cli::debug {

    /// options for the "debug" command
    struct Options {
        std::optional<std::string> dll;
        bool allow_fp16{true};
        int width{1920};
        int height{1080};

        float flow{0.85F};
        int multiplier{2};
        bool performance_mode{true};
        std::optional<std::string> gpu;

        std::filesystem::path path;
    };

    /// run the "debug" command
    /// @param opts the command options
    int run(const Options& opts);

}
