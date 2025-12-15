#include "instance.hpp"
#include "../configuration/config.hpp"
#include "../configuration/detection.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace lsfgvk;
using namespace lsfgvk::layer;

Root::Root() : identification(identify()) {
    this->tick();

    if (!this->profile.has_value())
        return;

    std::cerr << "lsfg-vk: using profile with name '" << this->profile->name << "' ";
    switch (this->identType) {
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
}

bool Root::tick() {
    auto res = this->config.tick();
    if (!res)
        return false;

    // try to find a profile
    const auto& detec = findProfile(this->config, identification);
    if (!detec.has_value())
        return this->profile.has_value();

    this->identType = detec->first;
    this->profile = detec->second;
    return true;
}

std::vector<const char*> Root::instanceExtensions() const {
    if (!this->profile.has_value())
        return {};

    return {
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_external_semaphore_capabilities"
    };
}

std::vector<const char*> Root::deviceExtensions() const {
    if (!this->profile.has_value())
        return {};

    return {
        "VK_KHR_external_memory",
        "VK_KHR_external_memory_fd",
        "VK_KHR_external_semaphore",
        "VK_KHR_external_semaphore_fd",
        "VK_KHR_timeline_semaphore"
    };
}

namespace {
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

    lsfgvk::Instance createBackend(const Root& root) {
        const auto& conf = root.conf();
        const auto& profile = root.active();
        if (!profile.has_value()) // should not happen
            throw lsfgvk::error("no profile active");

        return {
            [profile = *profile](
                const std::string& deviceName,
                std::pair<const std::string&, const std::string&> ids,
                const std::optional<std::string>& pci
            ) {
                if (!profile.gpu)
                    return true;

                return (deviceName == *profile.gpu)
                    || (ids.first + ":" + ids.second == *profile.gpu)
                    || (pci && *pci == *profile.gpu);
            },
            conf.dll.value_or(findShaderDll()),
            conf.allow_fp16
        };
    }
}

layer::Instance::Instance(const Root& root, vk::Vulkan vk)
        : vk(std::move(vk))/*, backend(createBackend(layer))*/ {
}
