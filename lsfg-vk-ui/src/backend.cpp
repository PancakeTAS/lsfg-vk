/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <QStringListModel>
#include <QStringList>
#include <QString>

#include "backend.hpp"
#include "utils.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/configuration/config.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <thread>

using namespace lsfgvk;
using namespace lsfgvk::ui;

Backend::Backend() {
    // Load existing configuration
    ls::ConfigFile config{};

    auto path = ls::findConfigurationFile();
    if (std::filesystem::exists(path)) {
        try {
            config = ls::ConfigFile(path);
        } catch (const std::exception&) {
            std::cerr << "The existing configuration file is invalid, "
                << "it has been backed up to '.old'\n";
            std::filesystem::rename(path, path.string() + ".old");
        }
    }

    this->m_global = config.global();
    this->m_profiles = config.profiles();

    // Create gpu list
    this->m_gpu_list = ui::queryGPUs();

    // Create profile list model
    QStringList profiles;
    for (const auto& profile : this->m_profiles)
        profiles.append(QString::fromStdString(profile.name));

    this->m_profile_list_model = new QStringListModel(profiles, this);

    // Create active_in list models
    this->m_active_in_list_models.reserve(this->m_profiles.size());
    for (const auto& profile : this->m_profiles) {
        QStringList active_in;
        for (const auto& path : profile.active_in)
            active_in.append(QString::fromStdString(path));

        this->m_active_in_list_models.push_back(new QStringListModel(active_in, this));
    }

    // Try to select first profile
    if (!this->m_profiles.empty())
        this->m_profile_index = 0;

    // Spawn saving thread
    std::thread([this, path]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (!this->m_dirty.exchange(false))
                continue;

            ls::ConfigFile config{};
            config.global() = this->m_global;
            config.profiles() = this->m_profiles;

            try {
                std::filesystem::create_directories(path.parent_path());
                if (!std::filesystem::exists(path.parent_path()))
                    throw ls::error("Unable to create configuration directory");
                config.write(path);
            } catch (const std::exception& e) {
                std::cerr << "Unable to write configuration:\n- " << e.what() << "\n";
            }
        }
    }).detach();
}
