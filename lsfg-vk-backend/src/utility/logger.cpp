/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "logger.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

using namespace lsfgvk;
using namespace lsfgvk::logger;

namespace {
    /// Get the current minimum log level
    Level& currentLevel() {
        static Level level{Level::Debug};
        return level;
    }

    /// Format a log level as a string
    constexpr std::string_view formatLevel(Level level) {
        switch (level) {
            case Level::Debug:
                return "DEBUG";
            case Level::Info:
                return "INFO ";
            case Level::Warning:
                return "WARN ";
            case Level::Error:
                return "ERROR";
        }
    }
}

void logger::setLevel(Level level) {
#ifdef NDEBUG
    if (level == Level::Debug) {
        LOG_WARNING("Release builds do not support debug log level, defaulting to info");
        level = Level::Info;
    }
#endif
    currentLevel() = level;
}

void logger::log(Level level, const std::string& message) {
    if (level < currentLevel())
        return;

    std::ostringstream log;
    log << "(lsfg-vk) [" << formatLevel(level) << "] " << message << '\n';

    std::cerr << log.str();
}
