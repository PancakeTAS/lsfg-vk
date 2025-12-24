/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <filesystem>

namespace ls {

    /// find the location of the Lossless.dll
    /// @returns the path to the DLL
    /// @throws ls::error if the DLL could not be found
    std::filesystem::path findShaderDll();

}
