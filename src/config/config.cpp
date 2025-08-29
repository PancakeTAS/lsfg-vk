#include "config/config.hpp"
#include "common/exception.hpp"
#include "config/default_conf.hpp"
#include "utils/utils.hpp"

#define TOML_ENABLE_FORMATTERS 0 // NOLINT
#include <toml.hpp>

#include <vulkan/vulkan_core.h>

#include <filesystem>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <chrono>
#include <thread>
#include <string>

using namespace Config;

GlobalConfiguration Config::globalConf{};
std::optional<GameConfiguration> Config::currentConf{};

namespace {
    /// Turn a string into a VkPresentModeKHR enum value.
    VkPresentModeKHR into_present(const std::string& mode) {
        if (mode == "fifo" || mode == "vsync")
            return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
        if (mode == "mailbox")
            return VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
        if (mode == "immediate")
            return VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;
        return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    }
}

void Config::updateConfig(
        const std::string& file,
        const std::pair<std::string, std::string>& name) {
    // process unchecked legacy environment variables
    if (std::getenv("LSFG_LEGACY")) {
        const char* dll = std::getenv("LSFG_DLL_PATH");
        if (dll) globalConf.dll = std::string(dll);

        currentConf.emplace();
        const char* multiplier = std::getenv("LSFG_MULTIPLIER");
        if (multiplier) currentConf->multiplier = std::stoul(multiplier);
        const char* flow_scale = std::getenv("LSFG_FLOW_SCALE");
        if (flow_scale) currentConf->flowScale = std::stof(flow_scale);
        const char* performance = std::getenv("LSFG_PERFORMANCE_MODE");
        if (performance) currentConf->performance = std::string(performance) == "1";
        const char* hdr = std::getenv("LSFG_HDR_MODE");
        if (hdr) currentConf->hdr = std::string(hdr) == "1";
        const char* e_present = std::getenv("LSFG_EXPERIMENTAL_PRESENT_MODE");
        if (e_present) currentConf->e_present = into_present(std::string(e_present));

        return;
    }

    // ensure configuration file exists
    if (!std::filesystem::exists(file)) {
        std::cerr << "lsfg-vk: Placing default configuration file at " << file << '\n';
        const auto parent = std::filesystem::path(file).parent_path();
        if (!std::filesystem::exists(parent))
            if (!std::filesystem::create_directories(parent))
                throw std::runtime_error("Unable to create configuration directory at " + parent.string());

        std::ofstream out(file);
        if (!out.is_open())
            throw std::runtime_error("Unable to create configuration file at " + file);
        out << DEFAULT_CONFIG;
        out.close();
    }

    // parse config file
    toml::table config{};
    try {
        config = toml::parse_file(file);

        const auto* version = config.get_as<toml::value<int64_t>>("version");
        if (!version || *version != 1)
            throw std::runtime_error("Configuration file version is not supported, expected 1");
    } catch (const std::exception& e) {
        throw LSFG::rethrowable_error("Unable to parse configuration file", e);
    }

    // parse global configuration
    Config::globalConf = {
        .config_file = file,
        .timestamp = std::filesystem::last_write_time(file)
    };
    if (const auto* global = config.get_as<toml::table>("global")) {
        if (const auto* val = global->get_as<toml::value<std::string>>("dll"))
            globalConf.dll = val->get();
        if (const auto* val = global->get_as<toml::value<bool>>("no_fp16"))
            globalConf.no_fp16 = val->get();
    }

    // parse game-specific configuration
    std::optional<GameConfiguration> gameConf;
    if (const auto* games = config["game"].as_array()) {
        for (auto&& elem : *games) {
            if (!elem.is_table())
                throw std::runtime_error("Invalid game configuration entry");
            const auto* game = elem.as_table();

            const auto* exe = game->at("exe").value_or("?");
            if (!name.first.ends_with(exe) && name.second != exe)
                continue;

            gameConf = Config::currentConf;
            if (!gameConf.has_value()) gameConf.emplace();

            if (const auto* val = game->get_as<toml::value<int64_t>>("multiplier"))
                gameConf->multiplier = static_cast<size_t>(val->get());
            if (const auto* val = game->get_as<toml::value<double>>("flow_scale"))
                gameConf->flowScale = static_cast<float>(val->get());
            if (const auto* val = game->get_as<toml::value<bool>>("performance_mode"))
                gameConf->performance = val->get();
            if (const auto* val = game->get_as<toml::value<bool>>("hdr_mode"))
                gameConf->hdr = val->get();
            if (const auto* val = game->get_as<toml::value<std::string>>("experimental_present_mode"))
                gameConf->e_present = into_present(val->get());

            break;
        }
    }

    if (!gameConf.has_value()) {
        if (Config::currentConf.has_value()) // print log message only if previously enabled
            std::cerr << "lsfg-vk: Configuration entry disappeared, disabling.\n";
        Config::currentConf.reset();
        return;
    }

    Config::currentConf = *gameConf;

    // print updated config info
    std::cerr << "lsfg-vk: Loaded configuration for " << name.first << ":\n";
    if (!globalConf.dll.empty()) std::cerr << "  Using DLL from: " << globalConf.dll << '\n';
    if (globalConf.no_fp16) std::cerr << "  FP16 Acceleration: Force-disabled\n";
    std::cerr << "  Multiplier: " << gameConf->multiplier << '\n';
    std::cerr << "  Flow Scale: " << gameConf->flowScale << '\n';
    std::cerr << "  Performance Mode: " << (gameConf->performance ? "Enabled" : "Disabled") << '\n';
    std::cerr << "  HDR Mode: " << (gameConf->hdr ? "Enabled" : "Disabled") << '\n';
    if (gameConf->e_present != 2) std::cerr << "  ! Present Mode: " << gameConf->e_present << '\n';
}

bool Config::checkStatus() {
    // check if config is up-to-date
    auto& globalConf = Config::globalConf;
    if (globalConf.config_file.empty())
        return true;
    if (!std::filesystem::exists(globalConf.config_file))
        return true; // ignore deletion
    if (std::filesystem::last_write_time(globalConf.config_file) == globalConf.timestamp)
        return true;

    // reload config
    std::cerr << "lsfg-vk: Rereading configuration, as it is no longer valid.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(73));
    try {
        Config::updateConfig(Utils::getConfigFile(), Utils::getProcessName());
    } catch (const std::exception& e) {
        std::cerr << "lsfg-vk: Failed to update configuration, continuing using old:\n";
        std::cerr << "- " << e.what() << '\n';
    }

    return false;
}
