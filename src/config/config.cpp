#include "config/config.hpp"

using namespace Config;

const Configuration defaultConf{
    .enable = false
};

bool Config::loadAndWatchConfig(const std::string& file) {
    return false;
}

Configuration Config::getConfig(std::string_view name) {
    return defaultConf;
}
