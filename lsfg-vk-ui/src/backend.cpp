#include "backend.hpp"
#include "lsfg-vk-common/configuration/config.hpp"

#include <QStringListModel>
#include <QStringList>
#include <QString>
#include <chrono>
#include <iostream>
#include <thread>

using namespace lsfgvk::ui;

Backend::Backend() {
    // load configuration
    ls::Configuration config;
    config.reload();

    this->m_global = config.getGlobalConf();
    this->m_profiles = config.getProfiles();

    // create profile list model
    QStringList profiles; // NOLINT (IWYU)
    for (const auto& profile : this->m_profiles)
        profiles.append(QString::fromStdString(profile.name));

    this->m_profile_list_model = new QStringListModel(profiles, this);

    // try to select first profile
    if (!this->m_profiles.empty())
        this->m_profile_index = 0;

    // spawn saving thread
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (!this->m_dirty.exchange(false))
                continue;

            std::cerr << "configuration updated >:3" << '\n';
        }
    }).detach();
}
