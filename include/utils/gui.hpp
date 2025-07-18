#pragma once

#include <string>

namespace Utils {

    ///
    /// Display error gui and exit
    ///
    /// @param message The error message to display in the GUI.
    ///
    [[noreturn]] void showErrorGui(const std::string& message);

}
