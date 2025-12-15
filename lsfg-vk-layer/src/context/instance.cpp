#include "instance.hpp"
#include "../configuration/config.hpp"
#include "../configuration/detection.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace lsfgvk;
using namespace lsfgvk::layer;

namespace {
    // find the shader dll
    std::filesystem::path findShaderDll() {
        const std::vector<std::filesystem::path> FRAGMENTS{{
            ".local/share/Steam/steamapps/common",
            ".steam/steam/steamapps/common",
            ".steam/debian-installation/steamapps/common",
            ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common",
            "snap/steam/common/.local/share/Steam/steamapps/common"
        }};

        // check XDG overridden location
        const char* xdgPath = std::getenv("XDG_DATA_HOME");
        if (xdgPath && *xdgPath != '\0') {
            auto base = std::filesystem::path(xdgPath);

            for (const auto& frag : FRAGMENTS) {
                auto full = base / frag / "Lossless Scaling" / "Lossless.dll";
                if (std::filesystem::exists(full))
                    return full;
            }
        }

        // check home directory
        const char* homePath = std::getenv("HOME");
        if (homePath && *homePath != '\0') {
            auto base = std::filesystem::path(homePath);

            for (const auto& frag : FRAGMENTS) {
                auto full = base / frag / "Lossless Scaling" / "Lossless.dll";
                if (std::filesystem::exists(full))
                    return full;
            }
        }

        // fallback to same directory
        auto local = std::filesystem::current_path() / "Lossless.dll";
        if (std::filesystem::exists(local))
            return local;

        throw lsfgvk::error("unable to locate Lossless.dll, please set the path in the configuration");
    }
}

Root::Root() {
    // find active profile
    this->config.reload();
    const auto& profile = findProfile(this->config, identify());
    if (!profile.has_value())
        return;

    this->active_profile = profile->second;

    std::cerr << "lsfg-vk: using profile with name '" << this->active_profile->name << "' ";
    switch (profile->first) {
        case IdentType::OVERRIDE:
            std::cerr << "(identified via override)\n";
            break;
        case IdentType::EXECUTABLE:
            std::cerr << "(identified via executable)\n";
            break;
        case IdentType::WINE_EXECUTABLE:
            std::cerr << "(identified via wine executable)\n";
            break;
        case IdentType::PROCESS_NAME:
            std::cerr << "(identified via process name)\n";
            break;
    }

    // create backend
    /*const auto& global = this->config.getGlobalConf();
    this->backend.emplace(
        [gpu = profile->second.gpu](
            const std::string& deviceName,
            std::pair<const std::string&, const std::string&> ids,
            const std::optional<std::string>& pci
        ) {
            if (!gpu)
                return true;

            return (deviceName == *gpu)
                || (ids.first + ":" + ids.second == *gpu)
                || (pci && *pci == *gpu);
        },
        global.dll.value_or(findShaderDll()),
        global.allow_fp16
    );*/
}

std::vector<const char*> Root::instanceExtensions() const {
    if (!this->active_profile.has_value())
        return {};

    return {
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_external_semaphore_capabilities"
    };
}

std::vector<const char*> Root::deviceExtensions() const {
    if (!this->active_profile.has_value())
        return {};

    return {
        "VK_KHR_external_memory",
        "VK_KHR_external_memory_fd",
        "VK_KHR_external_semaphore",
        "VK_KHR_external_semaphore_fd",
        "VK_KHR_timeline_semaphore"
    };
}
