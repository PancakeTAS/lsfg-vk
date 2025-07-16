#include "config/config.hpp"
#include "common/exception.hpp"

#include <filesystem>
#include <optional>
#include <toml11/find.hpp>
#include <toml11/parser.hpp>
#include <toml.hpp>

#include <unordered_map>
#include <string_view>
#include <exception>
#include <stdexcept>
#include <cstddef>
#include <utility>
#include <atomic>
#include <memory>
#include <string>

using namespace Config;

namespace {
    Configuration globalConf{};
    std::optional<std::unordered_map<std::string, Configuration>> gameConfs;
}

bool Config::loadAndWatchConfig(const std::string& file) {
    // parse config file
    toml::value toml{};
    if (std::filesystem::exists(file)) {
        try {
            toml = toml::parse(file);
        } catch (const std::exception& e) {
            throw LSFG::rethrowable_error("Unable to parse configuration file", e);
        }
    }

    // parse global configuration
    auto& global = globalConf;
    const toml::value globalTable = toml::find_or_default<toml::table>(toml, "global");
    global.enable = toml::find_or(globalTable, "enable", false);
    global.dll = toml::find_or(globalTable, "dll", std::string());
    global.multiplier = toml::find_or(globalTable, "multiplier", size_t(2));
    global.flowScale = toml::find_or(globalTable, "flow_scale", 1.0F);
    global.performance = toml::find_or(globalTable, "performance_mode", false);
    global.hdr = toml::find_or(globalTable, "hdr_mode", false);
    global.valid = std::make_shared<std::atomic_bool>(true);

    // validate global configuration
    if (global.multiplier < 2)
        throw std::runtime_error("Multiplier cannot be less than 2");
    if (global.flowScale < 0.25F || global.flowScale > 1.0F)
        throw std::runtime_error("Flow scale must be between 0.25 and 1.0");

    // parse game-specific configuration
    auto& games = gameConfs.emplace();
    const toml::value gamesList = toml::find_or_default<toml::array>(toml, "game");
    for (const auto& gameTable : gamesList.as_array()) {
        if (!gameTable.is_table())
            throw std::runtime_error("Invalid game configuration entry");
        if (!gameTable.contains("exe"))
            throw std::runtime_error("Game override missing 'exe' field");

        const std::string exe = toml::find<std::string>(gameTable, "exe");
        Configuration game{
            .enable = toml::find_or(gameTable, "enable", global.enable),
            .dll = toml::find_or(gameTable, "dll", global.dll),
            .multiplier = toml::find_or(gameTable, "multiplier", global.multiplier),
            .flowScale = toml::find_or(gameTable, "flow_scale", global.flowScale),
            .performance = toml::find_or(gameTable, "performance_mode", global.performance),
            .hdr = toml::find_or(gameTable, "hdr_mode", global.hdr),
            .valid = global.valid // only need a single validity flag
        };

        // validate the configuration
        if (game.multiplier < 2)
            throw std::runtime_error("Multiplier cannot be less than 2");
        if (game.flowScale < 0.25F || game.flowScale > 1.0F)
            throw std::runtime_error("Flow scale must be between 0.25 and 1.0");
        games[exe] = std::move(game);
    }

    // prepare config watcher
    // (TODO)

    return false;
}

Configuration Config::getConfig(std::string_view name) {
    if (name.empty() || !gameConfs.has_value())
        return globalConf;

    const auto& games = *gameConfs;
    auto it = games.find(std::string(name));
    if (it != games.end())
        return it->second;

    return globalConf;
}
