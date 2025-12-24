/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ls {

    /// global configuration
    struct GlobalConf {
        /// optional dll override
        std::optional<std::string> dll;
        /// should fp16 be allowed
        bool allow_fp16;
    };

    /// pacing methods
    enum class Pacing {
        /// do not perform any pacing (vsync+novrr)
        None
    };

    /// game profile configuration
    struct GameConf {
        /// name of the profile
        std::string name{"Profile"};
        /// optional activation string/array
        std::vector<std::string> active_in;
        /// gpu to use (in case of multiple)
        std::optional<std::string> gpu;
        /// multiplier for frame generation
        size_t multiplier{2};
        /// non-inverted flow scale
        float flow_scale{1.00F};
        /// use performance mode
        bool performance_mode{false};
        /// pacing method
        Pacing pacing{Pacing::None};
    };

    /// parsed configuration file
    class ConfigFile {
    public:
        /// create a default configuration file at the given path
        /// @param path path to configuration file
        /// @throws ls::error on failure
        static void createDefaultConfigFile(const std::filesystem::path& path);

        /// load the default configuration
        /// @throws ls::error on failure
        ConfigFile();
        /// load configuration from file
        /// @param path path to configuration file
        /// @throws ls::error on failure
        ConfigFile(const std::filesystem::path& path);

        /// get the global configuration
        /// @return global configuration
        [[nodiscard]] auto& global() { return this->globalConf; }
        /// get the game profiles
        /// @return list of game profiles
        [[nodiscard]] auto& profiles() { return this->profileConfs; }

        /// get the global configuration
        /// @return global configuration
        [[nodiscard]] const auto& global() const { return this->globalConf; }
        /// get the game profiles
        /// @return list of game profiles
        [[nodiscard]] const auto& profiles() const { return this->profileConfs; }

        /// write the configuration back to file
        /// @param path path to configuration file
        /// @throws ls::error on failure
        void write(const std::filesystem::path& path) const;
    private:
        GlobalConf globalConf{};
        std::vector<GameConf> profileConfs;
    };

    /// configuration watcher with additional environment support
    class WatchedConfig {
    public:
        /// create a new configuration watcher
        /// @throws ls::error on failure
        WatchedConfig();

        /// reload the configuration from disk if it has changed
        /// @throws ls::error on failure
        /// @return true if the configuration was reloaded
        bool update();

        /// access the underlying configuration file
        /// @return configuration file
        [[nodiscard]] const auto& get() const { return this->configFile; }
    private:
        ConfigFile configFile;

        std::filesystem::path path;
        std::chrono::time_point<std::chrono::file_clock> last_timestamp;
    };

    /// find the configuration file in the most common locations
    /// @return path to configuration file
    std::filesystem::path findConfigurationFile();

}
