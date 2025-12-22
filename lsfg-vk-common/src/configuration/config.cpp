#include "lsfg-vk-common/configuration/config.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#define TOML_ENABLE_FORMATTERS 0
#include <toml.hpp>

using namespace ls;

namespace {
     constexpr char const* DEFAULT_CONFIG = R"(version = 2

[global]
# dll = '/media/games/Lossless Scaling/Lossless.dll' # if you don't have LS in the default location
allow_fp16 = true # this will improve give a MASSIVE performance boost on AMD, but be super slow on older (!) NVIDIA GPUs

[[profile]]
name = "4x FG / 85% [Performance]"
active_in = [ # see the wiki for more info
    'vkcube',
    'vkcubepp'
]
# gpu = 'NVIDIA GeForce RTX 5080' # see the wiki for more info
multiplier = 4
flow_scale = 0.85
performance_mode = true
pacing = 'none' # see the wiki for more info

[[profile]]
name = "2x FG / 100%"
active_in = 'GenshinImpact.exe'
gpu = 'NVIDIA GeForce RTX 5080'
multiplier = 2
)";

    /// parse an activity array from toml value
    std::vector<std::string> activityFromString(const toml::node_view<const toml::node>& val) {
        std::vector<std::string> active_in{};

        if (const auto& as_str = val.value<std::string>()) {
            active_in.push_back(*as_str);
        }

        if (const auto& as_arr = val.as_array()) {
            for (const auto& item : *as_arr) {
                if (const auto& item_str = item.value<std::string>())
                    active_in.push_back(*item_str);
            }
        }

        return active_in;
    }
    /// parse a pacing method from string
    Pacing parcingFromString(const std::string& str) {
        if (str == "none")
            return Pacing::None;
        throw ls::error("unknown pacing method: " + str);
    }
    /// try to find the config
    std::filesystem::path findPath() {
        // always honor LSFGVK_CONFIG if set
        const char* envPath = std::getenv("LSFGVK_CONFIG");
        if (envPath && *envPath != '\0')
            return{envPath};

        // then check the XDG overriden location
        const char* xdgPath = std::getenv("XDG_CONFIG_HOME");
        if (xdgPath && *xdgPath != '\0')
            return std::filesystem::path(xdgPath)
                / "lsfg-vk" / "conf.toml";

        // fallback to typical user home
        const char* homePath = std::getenv("HOME");
        if (homePath && *homePath != '\0')
            return std::filesystem::path(homePath)
                / ".config" / "lsfg-vk" / "conf.toml";

        // finally, use system-wide config
        return "/etc/lsfg-vk/conf.toml";
    }
    /// parse the global configuration
    GlobalConf parseGlobalConf(const toml::table& tbl) {
        const GlobalConf conf{
            .dll = tbl["dll"].value<std::string>(),
            .allow_fp16 = tbl["allow_fp16"].value_or(true)
        };

        if (conf.dll && !std::filesystem::exists(*conf.dll))
            throw ls::error("path to dll is invalid");

        return conf;
    }
    /// parse a game profile configuration
    GameConf parseGameConf(const toml::table& tbl) {
        const GameConf conf{
            .name = tbl["name"].value_or<std::string>("unnamed"),
            .active_in = activityFromString(tbl["active_in"]),
            .gpu = tbl["gpu"].value<std::string>(),
            .multiplier = tbl["multiplier"].value_or(2U),
            .flow_scale = tbl["flow_scale"].value_or(1.0F),
            .performance_mode = tbl["performance_mode"].value_or(false),
            .pacing = parcingFromString(tbl["pacing"].value_or<std::string>("none"))
        };

        if (conf.multiplier <= 1)
            throw ls::error("multiplier must be greater than 1");
        if (conf.flow_scale < 0.25F || conf.flow_scale > 1.0F)
            throw ls::error("flow_scale must be between 0.25 and 1.0");

        return conf;
    }
    /// parse the global configuration from the environment
    GlobalConf parseGlobalConfFromEnv() {
        GlobalConf conf{
            .dll = std::nullopt,
            .allow_fp16 = true
        };

        const char* dll = std::getenv("LSFGVK_DLL_PATH");
        if (dll && *dll != '\0')
            conf.dll = std::string(dll);
        const char* no_fp16 = std::getenv("LSFGVK_NO_FP16");
        if (no_fp16 && *no_fp16 != '\0')
            conf.allow_fp16 = std::string(no_fp16) != "1";

        if (conf.dll && !std::filesystem::exists(*conf.dll))
            throw ls::error("path to dll is invalid");

        return conf;
    }
    /// parse a game profile configuration from the environment
    GameConf parseGameConfFromEnv() {
        GameConf conf{
            .name = "(environment)",
            .active_in = {},
            .gpu = std::nullopt,

            .multiplier = 2,
            .flow_scale = 1.0F,
            .performance_mode = false,
            .pacing = Pacing::None
        };

        const char* gpu = std::getenv("LSFGVK_GPU");
        if (gpu) conf.gpu = std::string(gpu);
        const char* multiplier = std::getenv("LSFGVK_MULTIPLIER");
        if (multiplier) conf.multiplier = static_cast<size_t>(std::stoul(multiplier));
        const char* flow_scale = std::getenv("LSFGVK_FLOW_SCALE");
        if (flow_scale) conf.flow_scale = std::stof(flow_scale);
        const char* performance = std::getenv("LSFGVK_PERFORMANCE_MODE");
        if (performance) conf.performance_mode = std::string(performance) == "1";
        const char* pacing = std::getenv("LSFGVK_PACING");
        if (pacing) conf.pacing = parcingFromString(std::string(pacing));

        if (conf.multiplier <= 1)
            throw ls::error("multiplier must be greater than 1");
        if (conf.flow_scale < 0.25F || conf.flow_scale > 1.0F)
            throw ls::error("flow_scale must be between 0.25 and 1.0");

        return conf;
    }
}

Configuration::Configuration() :
        path(findPath()),
        from_env(std::getenv("LSFGVK_ENV") != nullptr) {
    if (this->from_env) {
        this->global = parseGlobalConfFromEnv();
        this->profiles.push_back(parseGameConfFromEnv());
        return;
    }

    if (std::filesystem::exists(this->path))
        return;

    try {
        std::filesystem::create_directories(this->path.parent_path());
        if (!std::filesystem::exists(this->path.parent_path()))
            throw ls::error("unable to create configuration directory");

        std::ofstream ofs(this->path);
        if (!ofs.is_open())
            throw ls::error("unable to create default configuration file");

        ofs << DEFAULT_CONFIG;
        ofs.close();
    } catch (const std::filesystem::filesystem_error& e) {
        throw ls::error("unable to create default configuration file", e);
    }
}

bool Configuration::isUpToDate() {
    if (this->from_env)
        return true;

    try {
        return std::filesystem::last_write_time(this->path) == this->timestamp;
    } catch (const std::filesystem::filesystem_error& e) {
        throw ls::error("unable to access configuration file", e);
    }
}

void Configuration::reload() {
    try {
        this->timestamp = std::filesystem::last_write_time(this->path);
    } catch (const std::filesystem::filesystem_error& e) {
        throw ls::error("unable to access configuration file", e);
    }

    GlobalConf global{};
    std::vector<GameConf> profiles{};

    toml::table tbl;
    try {
        tbl = toml::parse_file(this->path.string());
    } catch (const toml::parse_error& e) {
        throw ls::error("unable to parse configuration", e);
    }

    auto vrs = tbl["version"];
    if (!vrs || !vrs.is_integer() || *vrs.as_integer() != 2)
        throw ls::error("unsupported configuration version");

    auto gbl = tbl["global"];
    if (gbl && gbl.is_table()) {
        global = parseGlobalConf(*gbl.as_table());
    }

    auto pfls = tbl["profile"];
    if (pfls && pfls.is_array_of_tables()) {
        for (const auto& pfl : *pfls.as_array())
            profiles.push_back(parseGameConf(*pfl.as_table()));
    }

    this->global = std::move(global);
    this->profiles = std::move(profiles);
}
