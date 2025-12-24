#include <QStringListModel>
#include <QStringList>
#include <QString>

#include "backend.hpp"
#include "utils.hpp"
#include "lsfg-vk-common/configuration/config.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <thread>

using namespace lsfgvk;
using namespace lsfgvk::ui;

Backend::Backend() {
    // load configuration
    ls::ConfigFile config{};

    auto path = ls::findConfigurationFile();
    if (std::filesystem::exists(path)) {
        try {
            config = ls::ConfigFile(path);
        } catch (const std::exception&) {
            std::cerr << "the configuration file is invalid, it has been backed up to '.old'\n";
            std::filesystem::rename(path, path.string() + ".old");
        }
    }

    this->m_global = config.global();
    this->m_profiles = config.profiles();

    // create gpu list
    this->m_gpu_list = ui::getAvailableGPUs();

    // create profile list model
    QStringList profiles; // NOLINT (IWYU)
    for (const auto& profile : this->m_profiles)
        profiles.append(QString::fromStdString(profile.name));

    this->m_profile_list_model = new QStringListModel(profiles, this);

    // create active_in list models
    this->m_active_in_list_models.reserve(this->m_profiles.size());
    for (const auto& profile : this->m_profiles) {
        QStringList active_in; // NOLINT (IWYU)
        for (const auto& path : profile.active_in)
            active_in.append(QString::fromStdString(path));

        this->m_active_in_list_models.push_back(new QStringListModel(active_in, this));
    }

    // try to select first profile
    if (!this->m_profiles.empty())
        this->m_profile_index = 0;

    // spawn saving thread
    std::thread([this, path]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (!this->m_dirty.exchange(false))
                continue;

            ls::ConfigFile config{};
            config.global() = this->m_global;
            config.profiles() = this->m_profiles;

            try {
                config.write(path);
            } catch (const std::exception& e) {
                std::cerr << "unable to write configuration:\n- " << e.what() << "\n";
            }
        }
    }).detach();
}
