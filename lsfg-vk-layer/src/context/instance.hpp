#pragma once

#include "../configuration/config.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/helpers/pointers.hpp"

#include <optional>
#include <vector>

namespace lsfgvk::layer {

    /// root context of the lsfg-vk layer
    class Root {
    public:
        /// create the lsfg-vk root context
        /// @throws lsfgvk::error on failure
        Root();

        /// check if the layer is active
        /// @return true if active
        [[nodiscard]] bool active() const { return this->active_profile.has_value(); }

        /// required instance extensions
        /// @return list of extension names
        [[nodiscard]] std::vector<const char*> instanceExtensions() const;
        /// required device extensions
        /// @return list of extension names
        [[nodiscard]] std::vector<const char*> deviceExtensions() const;
    private:
        Configuration config;
        std::optional<GameConf> active_profile;

        ls::lazy<lsfgvk::Instance> backend;
    };

}
