/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <optional>
#include <string>

namespace lsfgvk::cli::benchmark {

    /// options for the "benchmark" command
    struct Options {
        std::optional<std::string> dll;
        bool allow_fp16{true};
        int width{1920};
        int height{1080};

        float flow{0.85F};
        int multiplier{2};
        bool performance_mode{true};
        std::optional<std::string> gpu;

        int duration{10};
    };

    /// run the "benchmark" command
    /// @param opts the command options
    int run(const Options& opts);

}
