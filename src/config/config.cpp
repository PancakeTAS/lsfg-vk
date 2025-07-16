#include "config/config.hpp"
#include "common/exception.hpp"

#include <linux/limits.h>
#include <sys/inotify.h>
#include <toml11/find.hpp>
#include <toml11/parser.hpp>
#include <toml.hpp>
#include <unistd.h>

#include <unordered_map>
#include <string_view>
#include <filesystem>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <cstddef>
#include <utility>
#include <cstring>
#include <chrono>
#include <thread>
#include <cerrno>
#include <atomic>
#include <memory>
#include <string>
#include <array>

using namespace Config;

namespace {
    Configuration globalConf{};
    std::optional<std::unordered_map<std::string, Configuration>> gameConfs;
}

bool Config::loadAndWatchConfig(const std::string& file) {
    if (!std::filesystem::exists(file))
        return false;

    // parse config file
    std::optional<toml::value> parsed;
    try {
        parsed.emplace(toml::parse(file));
    } catch (const std::exception& e) {
        throw LSFG::rethrowable_error("Unable to parse configuration file", e);
    }
    auto& toml = *parsed;

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
    std::thread([file = file, valid = global.valid]() {
        try {
            const int fd = inotify_init();
            if (fd < 0)
                throw std::runtime_error("Failed to initialize inotify\n"
                    "- " + std::string(strerror(errno)));

            const int wd = inotify_add_watch(fd, file.c_str(), IN_MODIFY | IN_CLOSE_WRITE);
            if (wd < 0) {
                close(fd);

                throw std::runtime_error("Failed to add inotify watch for " + file + "\n"
                    "- " + std::string(strerror(errno)));
            }

            // watch for changes
            std::optional<std::chrono::steady_clock::time_point> discard_until;

            std::array<char, (sizeof(inotify_event) + NAME_MAX + 1) * 20> buffer{};
            while (true) {
                const ssize_t len = read(fd, buffer.data(), buffer.size());
                if (len <= 0 && errno != EINTR) {
                    inotify_rm_watch(fd, wd);
                    close(fd);

                    throw std::runtime_error("Error reading inotify event\n"
                        "- " + std::string(strerror(errno)));
                }

                size_t i{};
                while (std::cmp_less(i, len)) {
                    auto* event = reinterpret_cast<inotify_event*>(&buffer.at(i));
                    i += sizeof(inotify_event) + event->len;
                    if (event->len <= 0)
                        continue;

                    // stall a bit, then mark as invalid
                    discard_until.emplace(std::chrono::steady_clock::now()
                        + std::chrono::milliseconds(500));
                }

                auto now = std::chrono::steady_clock::now();
                if (discard_until.has_value() && now > *discard_until) {
                    discard_until.reset();

                    // mark config as invalid
                    valid->store(false, std::memory_order_release);

                    // and wait until it has been marked as valid again
                    while (!valid->load(std::memory_order_acquire))
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: Error in config watcher thread:\n";
            std::cerr << "- " << e.what() << '\n';
        }
    }).detach();

    return true;
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
