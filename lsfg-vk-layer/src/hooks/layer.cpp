/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "layer.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/configuration/detection.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/paths.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include <stdlib.h>

using namespace lsfgvk;
using namespace lsfgvk::layer;

MyVkLayer::MyVkLayer() {
    const auto& profile = ls::findProfile(this->config.get(), ls::identify());
    if (!profile.has_value())
        return;

    std::cerr << "lsfg-vk: using profile with name '" << profile->second.name << "' ";
    switch (profile->first) {
        case ls::IdentType::OVERRIDE:
            std::cerr << "(identified via override)\n";
            break;
        case ls::IdentType::EXECUTABLE:
            std::cerr << "(identified via executable)\n";
            break;
        case ls::IdentType::WINE_EXECUTABLE:
            std::cerr << "(identified via wine executable)\n";
            break;
        case ls::IdentType::PROCESS_NAME:
            std::cerr << "(identified via process name)\n";
            break;
    }

    this->current_profile.emplace(profile->second);
}

bool MyVkLayer::update() {
    if (!this->config.update())
        return false;

    const auto& profile = findProfile(this->config.get(), ls::identify());
    if (profile.has_value())
        this->current_profile = profile->second;
    else
        this->current_profile = std::nullopt;

    return true;
}

backend::Instance& MyVkLayer::backend() {
    if (this->backend_instance.has_value())
        return this->backend_instance.mut();

    if (!this->current_profile.has_value())
        throw ls::error("attempted to get backend instance while layer is inactive");
    const auto& global = this->config.get().global();
    const auto& profile = *this->current_profile;

    setenv("DISABLE_LSFGVK", "1", 1);

    try {
        std::string dll{};
        if (global.dll.has_value())
            dll = *global.dll;
        else
            dll = ls::findShaderDll();

        this->backend_instance.emplace(
            [gpu = profile.gpu](
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
            dll, global.allow_fp16
        );
    } catch (const std::exception& e) {
        unsetenv("DISABLE_LSFGVK");
        throw ls::error("failed to create backend instance", e);
    }

    unsetenv("DISABLE_LSFGVK");

    return this->backend_instance.mut();
}
