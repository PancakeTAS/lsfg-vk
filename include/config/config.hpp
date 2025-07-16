#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace Config {

    /// lsfg-vk configuration
    struct Configuration {
        /// Whether lsfg-vk should be loaded in the first place.
        bool enable{false};
        /// Path to Lossless.dll.
        std::string dll;

        /// The frame generation muliplier
        size_t multiplier{2};
        /// The internal flow scale factor
        float flowScale{1.0F};
        /// Whether performance mode is enabled
        bool performance{false};
        /// Whether HDR is enabled
        bool hdr{false};

        /// Atomic property to check if the configuration is valid or outdated.
        std::shared_ptr<std::atomic_bool> valid;
    };

    ///
    /// Load the config file and create a file watcher.
    ///
    /// @param file The path to the configuration file.
    /// @return Whether a configuration exists or not.
    ///
    /// @throws std::runtime_error if an error occurs while loading the configuration file.
    ///
    bool loadAndWatchConfig(const std::string& file);

    ///
    /// Get the configuration for a game.
    ///
    /// @param name The name of the executable to fetch.
    /// @return The configuration for the game or global configuration.
    ///
    /// @throws std::runtime_error if the configuration is invalid.
    ///
    Configuration getConfig(std::string_view name);

}
