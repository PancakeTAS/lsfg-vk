#pragma once

#include <vulkan/vulkan_core.h>

#include <filesystem>
#include <chrono>
#include <cstddef>
#include <string>

namespace Config {

    /// Global lsfg-vk configuration.
    struct GlobalConfiguration {
        /// Path to Lossless.dll.
        std::string dll;
        /// Whether FP16 is force-disabled
        bool no_fp16{false};

        /// Path to the configuration file.
        std::filesystem::path config_file;
        /// File timestamp of the configuration file
        std::chrono::time_point<std::chrono::file_clock> timestamp;
    };

    /// Per-application lsfg-vk configuration.
    struct GameConfiguration {
        /// The frame generation muliplier
        size_t multiplier{2};
        /// The internal flow scale factor
        float flowScale{1.0F};
        /// Whether performance mode is enabled
        bool performance{false};
        /// Whether HDR is enabled
        bool hdr{false};

        /// Experimental flag for overriding the synchronization method.
        VkPresentModeKHR e_present{ VK_PRESENT_MODE_FIFO_KHR };

    };

    /// Global configuration.
    extern GlobalConfiguration globalConf;
    /// Currently active configuration.
    extern std::optional<GameConfiguration> currentConf;

    ///
    /// Read the configuration file while preserving the previous configuration
    /// in case of an error.
    ///
    /// @param file The path to the configuration file.
    /// @param name The preset to activate
    ///
    /// @throws std::runtime_error if an error occurs while loading the configuration file.
    ///
    void updateConfig(
        const std::string& file,
        const std::pair<std::string, std::string>& name
    );

    ///
    /// Check if the configuration file is still up-to-date
    ///
    /// @return Whether the configuration is up-to-date or not.
    ///
    bool checkStatus();

}
