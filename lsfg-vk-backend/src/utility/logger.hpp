/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <sstream> // IWYU pragma: keep
#include <string>
#include <string_view> // IWYU pragma: keep

namespace lsfgvk::logger {

    /// Various levels for log messages
    enum class Level : uint8_t {
        /// Detailed debugging information
        Debug,
        /// General informational messages
        Info,
        /// Potentially problematic situations
        Warning,
        /// Irrecoverable errors
        Error
    };

    ///
    /// Set the minimum log level
    ///
    /// @param level Inclusive minimum log level
    ///
    void setLevel(Level level);

    ///
    /// Log a message
    ///
    /// @param level Log level
    /// @param message Log message
    ///
    void log(Level level, const std::string& message);

    // NOLINTBEGIN (macro parentheses)
    #define LOG(level, msg) { \
        std::ostringstream _oss; \
        _oss << msg; \
        lsfgvk::logger::log(level, _oss.str()); \
    }

    #define LOG_INFO(msg) LOG(lsfgvk::logger::Level::Info, msg)
    #define LOG_WARNING(msg) LOG(lsfgvk::logger::Level::Warning, msg)
    #define LOG_ERROR(msg) LOG(lsfgvk::logger::Level::Error, msg)
#ifdef NDEBUG
    #define LOG_DEBUG(msg)
#else
    #define LOG_DEBUG(msg) LOG(lsfgvk::logger::Level::Debug, msg)
#endif
    // NOLINTEND

}
