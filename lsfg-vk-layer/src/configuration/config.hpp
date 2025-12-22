#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace lsfgvk::layer {

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
        std::string name;
        /// optional activation string/array
        std::vector<std::string> active_in;
        /// gpu to use (in case of multiple)
        std::optional<std::string> gpu;
        /// multiplier for frame generation
        size_t multiplier;
        /// non-inverted flow scale
        float flow_scale;
        /// use performance mode
        bool performance_mode;
        /// pacing method
        Pacing pacing;
    };

    /// automatically updating configuration
    class Configuration {
    public:
        /// create a new configuration
        /// @throws ls::error on failure
        Configuration();

        /// check if the configuration is out of date
        /// @throws ls::error on failure
        /// @return true if the configuration is out of date
        bool isUpToDate();

        /// reload the configuration from disk
        /// @throws ls::error on failure
        void reload();

        /// get the global configuration
        /// @return global configuration
        [[nodiscard]] const GlobalConf& getGlobalConf() const { return global; }

        /// get the game profiles
        /// @return list of game profiles
        [[nodiscard]] const std::vector<GameConf>& getProfiles() const { return profiles; }
    private:
        std::filesystem::path path;
        std::chrono::time_point<std::chrono::file_clock> timestamp;
        bool from_env{};

        GlobalConf global;
        std::vector<GameConf> profiles;
    };

}
