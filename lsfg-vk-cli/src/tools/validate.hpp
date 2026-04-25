/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <optional>
#include <string>

namespace lsfgvk::cli::validate {

    ///
    /// Options for the "validate" command
    ///
    struct Options {
        std::optional<std::string> config;
    };

    ///
    /// Run the "validate" command
    ///
    /// @param opts Command options
    /// @return Exit code
    ///
    int run(const Options& opts);

}
