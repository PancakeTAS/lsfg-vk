#pragma once

#include <vulkan/vulkan_core.h>

#include <filesystem>
#include <chrono>
#include <cstddef>
#include <string>

namespace Config {

    /// lsfg-vk configuration
    struct Configuration {
        /// Whether lsfg-vk should be loaded in the first place.
        bool enable{false};
        /// Path to Lossless.dll.
        std::string dll;
        /// Whether FP16 is force-disabled
        bool no_fp16{false};

        /// The frame generation muliplier
        size_t multiplier{2};
        /// The internal flow scale factor
        float flowScale{1.0F};
        /// Whether performance mode is enabled
        bool performance{false};
        /// Whether HDR is enabled
        bool hdr{false};

        /// Experimental flag for overriding the synchronization method.
        VkPresentModeKHR e_present;

        /// Path to the configuration file.
        std::filesystem::path config_file;
        /// File timestamp of the configuration file
        std::chrono::time_point<std::chrono::file_clock> timestamp;
    };

    /// Active configuration. Must be set in main.cpp.
    extern Configuration activeConf;

    ///
    /// Read the configuration file while preserving the previous configuration
    /// in case of an error.
    ///
    /// @param file The path to the configuration file.
    ///
    /// @throws std::runtime_error if an error occurs while loading the configuration file.
    ///
    void updateConfig(const std::string& file);

    ///
    /// Get the configuration for a game.
    ///
    /// @param name The name of the executable to fetch.
    /// @return The configuration for the game or global configuration.
    ///
    /// @throws std::runtime_error if the configuration is invalid.
    ///
    Configuration getConfig(const std::pair<std::string, std::string>& name);

}
