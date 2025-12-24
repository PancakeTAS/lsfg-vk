/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <optional>
#include <string>

namespace lsfgvk::cli::validate {

    /// options for the "validate" command
    struct Options {
        std::optional<std::string> config;
    };

    /// run the "validate" command
    /// @param opts the command options
    int run(const Options& opts);

}
