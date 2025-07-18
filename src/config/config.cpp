#include "config/config.hpp"
#include "common/exception.hpp"

#include "config/default_conf.hpp"

#include <vulkan/vulkan_core.h>
#include <linux/limits.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <toml11/find.hpp>
#include <toml11/parser.hpp>
#include <toml.hpp>
#include <unistd.h>
#include <poll.h>

#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <fstream>
#include <cstddef>
#include <utility>
#include <cstring>
#include <vector>
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

Configuration Config::activeConf{};

namespace {
    [[noreturn]] void thread(
            const std::string& file,
            const std::shared_ptr<std::atomic_bool>& valid) {
        const int fd = inotify_init();
        if (fd < 0)
            throw std::runtime_error("Failed to initialize inotify:\n"
                "- " + std::string(strerror(errno)));

        const std::string parent = std::filesystem::path(file).parent_path().string();
        const int wd = inotify_add_watch(fd, parent.c_str(),
            IN_MODIFY | IN_CLOSE_WRITE | IN_MOVE_SELF);
        if (wd < 0) {
            close(fd);

            throw std::runtime_error("Failed to add inotify watch for " + parent + ":\n"
                "- " + std::string(strerror(errno)));
        }

        // watch for changes
        std::optional<std::chrono::steady_clock::time_point> discard_until;
        const std::string filename = std::filesystem::path(file).filename().string();

        std::array<char, (sizeof(inotify_event) + NAME_MAX + 1) * 20> buffer{};
        while (true) {
            // poll fd
            struct pollfd pfd{};
            pfd.fd = fd;
            pfd.events = POLLIN;
            const int pollRes = poll(&pfd, 1, 100);
            if (pollRes < 0 && errno != EINTR) {
                inotify_rm_watch(fd, wd);
                close(fd);

                throw std::runtime_error("Error polling inotify events:\n"
                    "- " + std::string(strerror(errno)));
            }

            // read fd if there are events
            const ssize_t len = pollRes == 0 ? 0 : read(fd, buffer.data(), buffer.size());
            if (len <= 0 && errno != EINTR && pollRes > 0) {
                inotify_rm_watch(fd, wd);
                close(fd);

                throw std::runtime_error("Error reading inotify events:\n"
                    "- " + std::string(strerror(errno)));
            }

            size_t i{};
            while (std::cmp_less(i, len)) {
                auto* event = reinterpret_cast<inotify_event*>(&buffer.at(i));
                i += sizeof(inotify_event) + event->len;
                if (event->len <= 0 || event->mask & IN_IGNORED)
                    continue;

                const std::string name(reinterpret_cast<char*>(event->name));
                if (name != filename)
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
    }
}

void Config::loadAndWatchConfig(const std::string& file) {
    globalConf.valid = std::make_shared<std::atomic_bool>(true);
    updateConfig(file);

    // prepare config watcher
    std::thread([file = file, valid = globalConf.valid]() {
        try {
            thread(file, valid);
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: Error in config watcher thread:\n";
            std::cerr << "- " << e.what() << '\n';
        }
    }).detach();
}

namespace {
    /// Turn a string into a VkPresentModeKHR enum value.
    VkPresentModeKHR into_present(const std::string& mode, VkPresentModeKHR defaultMode) {
        if (mode.empty())
            return defaultMode;
        if (mode == "fifo" || mode == "vsync")
            return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
        if (mode == "mailbox")
            return VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
        if (mode == "immediate")
            return VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;
        return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    }

    /// Parse environment variables from a string.
    std::vector<std::pair<std::string, std::string>> parse_env(const std::string& envs) {
        std::vector<std::pair<std::string, std::string>> vars{};
        const std::string env_str = envs + ' ';

        std::string current{};
        bool escape{false};
        for (const char c : env_str) {
            // toggle escape mode
            if (c == '\'') {
                escape = !escape;
                continue;
            }

            // parse variable
            if (c == ' ' && !escape) {
                if (current.empty())
                    continue;

                auto eq_pos = current.find('=');
                if (eq_pos == std::string::npos)
                    throw std::runtime_error("Invalid environment variable: " + current);

                std::string key = current.substr(0, eq_pos);
                std::string value = current.substr(eq_pos + 1);
                if (key.empty() || value.empty())
                    throw std::runtime_error("Invalid environment variable: " + current);

                vars.emplace_back(std::move(key), std::move(value));

                current.clear();
                continue;
            }

            current += c;
        }

        return vars;
    }
}

void Config::updateConfig(const std::string& file) {
    globalConf.valid->store(true, std::memory_order_relaxed);
    if (!std::filesystem::exists(file)) {
        std::cerr << "lsfg-vk: Placing default configuration file at " << file << '\n';
        std::ofstream out(file);
        if (!out.is_open())
            throw std::runtime_error("Unable to create configuration file at " + file);
        out << DEFAULT_CONFIG;
        out.close();
    }

    // parse config file
    std::optional<toml::value> parsed;
    try {
        parsed.emplace(toml::parse(file));
    } catch (const std::exception& e) {
        throw LSFG::rethrowable_error("Unable to parse configuration file", e);
    }
    auto& toml = *parsed;

    // parse global configuration
    const toml::value globalTable = toml::find_or_default<toml::table>(toml, "global");
    const Configuration global{
        .dll =   toml::find_or(globalTable, "dll", std::string()),
        .valid = globalConf.valid // use the same validity flag
    };

    // validate global configuration
    if (global.multiplier < 2)
        throw std::runtime_error("Global Multiplier cannot be less than 2");
    if (global.flowScale < 0.25F || global.flowScale > 1.0F)
        throw std::runtime_error("Flow scale must be between 0.25 and 1.0");

    // parse game-specific configuration
    std::unordered_map<std::string, Configuration> games;
    const toml::value gamesList = toml::find_or_default<toml::array>(toml, "game");
    for (const auto& gameTable : gamesList.as_array()) {
        if (!gameTable.is_table())
            throw std::runtime_error("Invalid game configuration entry");
        if (!gameTable.contains("exe"))
            throw std::runtime_error("Game override missing 'exe' field");

        const std::string exe = toml::find<std::string>(gameTable, "exe");
        Configuration game{
            .enable = true,
            .env = parse_env(toml::find_or(gameTable, "env", std::string())),
            .multiplier = toml::find_or(gameTable, "multiplier", global.multiplier),
            .flowScale = toml::find_or(gameTable, "flow_scale", global.flowScale),
            .performance = toml::find_or(gameTable, "performance_mode", global.performance),
            .hdr = toml::find_or(gameTable, "hdr_mode", global.hdr),
            .e_present =   into_present(
                toml::find_or(gameTable, "experimental_present_mode", ""),
                global.e_present),
            .e_fps_limit = toml::find_or(gameTable, "experimental_fps_limit", global.e_fps_limit),
            .valid = global.valid // only need a single validity flag
        };

        // validate the configuration
        if (game.multiplier < 1)
            throw std::runtime_error("Multiplier cannot be less than 1");
        if (game.flowScale < 0.25F || game.flowScale > 1.0F)
            throw std::runtime_error("Flow scale must be between 0.25 and 1.0");
        games[exe] = std::move(game);
    }

    // store configurations
    globalConf = global;
    gameConfs = std::move(games);
}

Configuration Config::getConfig(const std::pair<std::string, std::string>& name) {
    if (!gameConfs.has_value())
        return globalConf;

    const auto& games = *gameConfs;
    auto it = std::ranges::find_if(games, [&name](const auto& pair) {
        return name.first.ends_with(pair.first) || (name.second == pair.first);
    });
    if (it != games.end())
        return it->second;

    return globalConf;
}
