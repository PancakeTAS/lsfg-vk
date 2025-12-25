/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <QObject>
#include <QStringListModel>
#include <QString>

#include "lsfg-vk-common/configuration/config.hpp"

#include <atomic>
#include <utility>

#define getters public
#define setters public

namespace lsfgvk::ui {

    /// Class tying ui and configuration together
    class Backend : public QObject {
        Q_OBJECT

        Q_PROPERTY(QStringListModel* profiles READ calculateProfileListModel NOTIFY refreshUI)
        Q_PROPERTY(int profile_index READ getProfileIndex WRITE profileSelected NOTIFY refreshUI)

        Q_PROPERTY(QString dll READ getDll WRITE dllUpdated NOTIFY refreshUI)
        Q_PROPERTY(bool allow_fp16 READ getAllowFP16 WRITE allowFP16Updated NOTIFY refreshUI)

        Q_PROPERTY(bool available READ isValidProfileIndex NOTIFY refreshUI)
        Q_PROPERTY(QStringListModel* active_in READ calculateActiveInModel NOTIFY refreshUI)
        Q_PROPERTY(int active_in_index READ getActiveInIndex WRITE activeInSelected NOTIFY refreshUI)
        Q_PROPERTY(size_t multiplier READ getMultiplier WRITE multiplierUpdated NOTIFY refreshUI)
        Q_PROPERTY(float flow_scale READ getFlowScale WRITE flowScaleUpdated NOTIFY refreshUI)
        Q_PROPERTY(bool performance_mode READ getPerformanceMode WRITE performanceModeUpdated NOTIFY refreshUI)
        Q_PROPERTY(int pacing_mode READ getPacingMode WRITE pacingModeUpdated NOTIFY refreshUI)
        Q_PROPERTY(QStringList gpus READ calculateGPUList NOTIFY refreshUI)
        Q_PROPERTY(int gpu READ getGPU WRITE gpuUpdated NOTIFY refreshUI)

    public:
        explicit Backend();

    getters:
        [[nodiscard]] QStringListModel* calculateProfileListModel() const {
            return this->m_profile_list_model;
        }
        [[nodiscard]] int getProfileIndex() const {
            return this->m_profile_index;
        }

        [[nodiscard]] QString getDll() const {
            return QString::fromStdString(this->m_global.dll.value_or(""));
        }
        [[nodiscard]] bool getAllowFP16() const {
            return this->m_global.allow_fp16;
        }

#define VALIDATE_AND_GET_PROFILE(default) \
    if (!isValidProfileIndex()) return default; \
    auto& conf = this->m_profiles[static_cast<size_t>(this->m_profile_index)];

        [[nodiscard]] bool isValidProfileIndex() const {
            return this->m_profile_index >= 0 && std::cmp_less(this->m_profile_index, this->m_profiles.size());
        }
        [[nodiscard]] QStringListModel* calculateActiveInModel() const {
            if (!isValidProfileIndex()) return nullptr;
            return this->m_active_in_list_models.at(static_cast<size_t>(this->m_profile_index));
        }
        [[nodiscard]] int getActiveInIndex() const {
            if (!isValidProfileIndex()) return -1;
            return static_cast<int>(this->m_active_in_index);
        }

        [[nodiscard]] size_t getMultiplier() const {
            VALIDATE_AND_GET_PROFILE(2)
            return conf.multiplier;
        }
        [[nodiscard]] float getFlowScale() const {
            VALIDATE_AND_GET_PROFILE(1.0F)
            return conf.flow_scale;
        }
        [[nodiscard]] bool getPerformanceMode() const {
            VALIDATE_AND_GET_PROFILE(false)
            return conf.performance_mode;
        }
        [[nodiscard]] int getPacingMode() const {
            VALIDATE_AND_GET_PROFILE(0)
            switch (conf.pacing) {
                case ls::Pacing::None: return 0;
            }
            throw std::runtime_error("Unknown pacing type in backend");
        }
        [[nodiscard]] QStringList calculateGPUList() const {
            return this->m_gpu_list;
        }
        [[nodiscard]] int getGPU() const {
            VALIDATE_AND_GET_PROFILE(0)
            auto gpu = QString::fromStdString(conf.gpu.value_or("Default"));
            return static_cast<int>(this->m_gpu_list.indexOf(gpu));
        }

#undef VALIDATE_AND_GET_PROFILE

    setters:
        void profileSelected(int idx) {
            this->m_profile_index = idx;
            emit refreshUI();
        }

        void activeInSelected(int idx) {
            this->m_active_in_index = idx;
            emit refreshUI();
        }

#define MARK_DIRTY() \
    this->m_dirty.store(true, std::memory_order_relaxed); \
    emit refreshUI();

        void dllUpdated(const QString& dll) {
            auto& conf = this->m_global;
            if (dll.trimmed().isEmpty())
                conf.dll = std::nullopt;
            else
                conf.dll = dll.toStdString();
            MARK_DIRTY()
        }
        void allowFP16Updated(bool allow_fp16) {
            auto& conf = this->m_global;
            conf.allow_fp16 = allow_fp16;
            MARK_DIRTY()
        }

#define VALIDATE_AND_GET_PROFILE() \
    if (!isValidProfileIndex()) return; \
    auto& conf = this->m_profiles[static_cast<size_t>(this->m_profile_index)];

        void multiplierUpdated(size_t multiplier) {
            VALIDATE_AND_GET_PROFILE()
            conf.multiplier = multiplier;
            MARK_DIRTY()
        }
        void flowScaleUpdated(float flow_scale) {
            VALIDATE_AND_GET_PROFILE()
            conf.flow_scale = flow_scale;
            MARK_DIRTY()
        }
        void performanceModeUpdated(bool performance_mode) {
            VALIDATE_AND_GET_PROFILE()
            conf.performance_mode = performance_mode;
            MARK_DIRTY()
        }
        void pacingModeUpdated(int pacing_mode) {
            VALIDATE_AND_GET_PROFILE()
            if (pacing_mode == 0)
            switch (pacing_mode) {
                case 0:
                    conf.pacing = ls::Pacing::None;
                    break;
                default:
                    throw std::runtime_error("Unknown pacing mode in backend");
            }
            MARK_DIRTY()
        }
        void gpuUpdated(int gpu_idx) {
            VALIDATE_AND_GET_PROFILE()
            const auto& gpu = this->m_gpu_list.at(gpu_idx);
            if (gpu.trimmed().isEmpty() || gpu == "Default")
                conf.gpu = std::nullopt;
            else
                conf.gpu.emplace(gpu.toStdString());
            MARK_DIRTY()
        }

        Q_INVOKABLE void addActiveIn(const QString& name) {
            if (name.trimmed().isEmpty()) return;
            VALIDATE_AND_GET_PROFILE()
            auto& active_in = conf.active_in;
            active_in.push_back(name.toStdString());

            auto& model = this->m_active_in_list_models
                .at(static_cast<size_t>(this->m_profile_index));
            model->insertRow(model->rowCount());
            model->setData(model->index(model->rowCount() - 1), name);
            MARK_DIRTY()
        }
        Q_INVOKABLE void removeActiveIn() {
            VALIDATE_AND_GET_PROFILE()
            if (this->m_active_in_index < 0 || std::cmp_greater_equal(static_cast<size_t>(this->m_active_in_index), conf.active_in.size()))
                return;

            auto& active_in = conf.active_in;
            active_in.erase(active_in.begin() + this->m_active_in_index);
            auto& model = this->m_active_in_list_models
                .at(static_cast<size_t>(this->m_profile_index));
            model->removeRow(this->m_active_in_index);
            if (!active_in.empty())
                this->m_active_in_index = 0;
            else
                this->m_active_in_index = -1;
            MARK_DIRTY()
        }

        Q_INVOKABLE void createProfile(const QString& name) {
            if (name.trimmed().isEmpty()) return;

            ls::GameConf conf;
            conf.name = name.toStdString();
            this->m_profiles.push_back(std::move(conf));
            this->m_active_in_list_models.push_back(new QStringListModel({}, this));

            auto& model = this->m_profile_list_model;
            model->insertRow(model->rowCount());
            model->setData(model->index(model->rowCount() - 1), name);

            this->m_profile_index = static_cast<int>(this->m_profiles.size() - 1);
            MARK_DIRTY()
        }
        Q_INVOKABLE void renameProfile(const QString& name) {
            if (name.trimmed().isEmpty()) return;

            VALIDATE_AND_GET_PROFILE()
            conf.name = name.toStdString();
            auto& model = this->m_profile_list_model;
            model->setData(model->index(this->m_profile_index), name);
            MARK_DIRTY()
        }
        Q_INVOKABLE void deleteProfile() {
            if (!isValidProfileIndex())
                return;

            auto& profiles = this->m_profiles;
            profiles.erase(profiles.begin() + this->m_profile_index);
            auto& active_in_models = this->m_active_in_list_models;
            active_in_models.erase(active_in_models.begin() + this->m_profile_index);
            auto& model = this->m_profile_list_model;
            model->removeRow(this->m_profile_index);
            if (!this->m_profiles.empty())
                this->m_profile_index = 0;
            else
                this->m_profile_index = -1;
            MARK_DIRTY()
        }

#undef VALIDATE_AND_GET_PROFILE
#undef MARK_DIRTY

    signals:
        void refreshUI();

    private:
        ls::GlobalConf m_global;
        std::vector<ls::GameConf> m_profiles;

        QStringListModel* m_profile_list_model;
        int m_profile_index{-1};

        std::vector<QStringListModel*> m_active_in_list_models;
        int m_active_in_index{-1};

        QStringList m_gpu_list;

        std::atomic_bool m_dirty{false};
    };

}
