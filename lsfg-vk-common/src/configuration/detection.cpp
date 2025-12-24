/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "lsfg-vk-common/configuration/detection.hpp"
#include "lsfg-vk-common/configuration/config.hpp"

#include <array>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include <sys/types.h>

using namespace ls;

namespace {
    // try to match a profile by id
    std::optional<GameConf> match(const std::vector<GameConf>& profiles, const std::string& id) {
        for (const auto& profile : profiles)
            for (const auto& activation : profile.active_in)
                if (activation == id)
                    return profile;
        return std::nullopt;
    }
}

Identification ls::identify() {
    Identification id{};

    // fetch LSFGVK_PROFILE
    const char* override = std::getenv("LSFGVK_PROFILE");
    if (override && *override != '\0')
        id.override = std::string(override);

    // fetch process exe path
    std::array<char, 4096> buf{};
    const ssize_t len = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (len > 0) {
        buf.at(static_cast<size_t>(len)) = '\0';
        id.executable = std::string(buf.data());
    }

    // if running under wine, fetch the actual exe path
    if (id.executable.find("wine") != std::string::npos
        || id.executable.find("proton") != std::string::npos) {

        std::ifstream maps("/proc/self/maps");
        std::string line;
        while (maps.is_open() && std::getline(maps, line)) {
            if (!line.ends_with(".exe"))
                continue;

            size_t pos = line.find_last_of('/');
            if (pos == std::string::npos) {
                pos = line.find_last_of(' ');
                if (pos == std::string::npos)
                    continue;
            }
            pos += 1; // skip slash or space

            const std::string wine_executable = line.substr(pos);
            if (wine_executable.empty())
                continue;

            id.wine_executable = wine_executable;
            break;
        }
    }

    // fetch process name
    std::ifstream comm("/proc/self/comm");
    if (comm.is_open()) {
        comm.read(buf.data(), buf.size() - 1);
        buf.at(static_cast<size_t>(comm.gcount())) = '\0';

        id.process_name = std::string(buf.data());
        if (id.process_name.back() == '\n')
            id.process_name.pop_back();
    }

    return id;
}

std::optional<std::pair<IdentType, GameConf>> ls::findProfile(
        const ConfigFile& config, const Identification& id) {
    const auto& profiles = config.profiles();

    // check for the environment option first
    if (std::getenv("LSFGVK_ENV") != nullptr)
        return std::make_pair(IdentType::OVERRIDE, profiles.front());

    // then override first
    if (id.override.has_value()) {
        const auto profile = match(profiles, id.override.value());
        if (profile.has_value())
            return std::make_pair(IdentType::OVERRIDE, profile.value());
    }

    // then check executable
    const auto exe_profile = match(profiles, id.executable);
    if (exe_profile.has_value())
        return std::make_pair(IdentType::EXECUTABLE, exe_profile.value());

    // if present, check wine executable next
    if (id.wine_executable.has_value()) {
        const auto wine_profile = match(profiles, id.wine_executable.value());
        if (wine_profile.has_value())
            return std::make_pair(IdentType::WINE_EXECUTABLE, wine_profile.value());
    }

    // finally, fallback to process name
    if (!id.process_name.empty()) {
        const auto proc_profile = match(profiles, id.process_name);
        if (proc_profile.has_value())
            return std::make_pair(IdentType::PROCESS_NAME, proc_profile.value());
    }

    return std::nullopt;
}
