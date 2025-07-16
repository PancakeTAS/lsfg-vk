#include "config/config.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

#include <string.h> // NOLINT
#include <unistd.h>

namespace {
    /// Check if the library is preloaded or Vulkan loaded.
    bool isPreload() {
        const char* preload = std::getenv("LD_PRELOAD");
        if (!preload || *preload == '\0')
            return false;
        const std::string preload_str(preload);
        return preload_str.find("liblsfg-vk.so") != std::string::npos;
    }

    /// Check if a benchmark is requested.
    bool requestedBenchmark() {
        const char* benchmark = std::getenv("LSFG_BENCHMARK");
        if (!benchmark || *benchmark == '\0')
            return false;
        const std::string benchmark_str(benchmark);
        return benchmark_str == "1";
    }

    /// Get the process name
    std::string getProcessName() {
        std::array<char, 4096> exe{};
        const ssize_t exe_len = readlink("/proc/self/exe", exe.data(), exe.size() - 1);
        if (exe_len <= 0)
            return "Unknown Process";
        exe.at(static_cast<size_t>(exe_len)) = '\0';
        return{basename(exe.data())};
    }

    /// Get the config file
    std::string getConfigFile() {
        const char* configFile = std::getenv("LSFG_CONFIG");
        if (configFile && *configFile != '\0')
            return{configFile};
        const char* homePath = std::getenv("HOME");
        if (homePath && *homePath != '\0')
            return std::string(homePath) + "/.config/lsfg-vk.toml";
        return "/etc/lsfg-vk.toml";
    }

    __attribute__((constructor)) void lsfgvk_init() {
        if (isPreload()) {
            if (requestedBenchmark()) {
                // TODO: Call benchmark function.
                exit(0);
            }

            // TODO: health check, maybe?
            std::cerr << "lsfg-vk: This library is not meant to be preloaded, unless you are running a benchmark.\n";
            exit(1);
        }

        // read configuration (might block)
        const std::string file = getConfigFile();
        try {
            Config::loadAndWatchConfig(file);
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occured while trying to parse the configuration, exiting:" << '\n';
            std::cerr << "- " << e.what() << '\n';
            exit(0);
        }

        const std::string name = getProcessName();
        Config::Configuration conf{};
        try {
            conf = Config::getConfig(name);
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: The configuration for " << name << " is invalid, exiting." << '\n';
            std::cerr << e.what() << '\n';
            exit(0);
        }

        // exit silently if not enabled
        if (!conf.enable)
            return;

        // print config
        std::cerr << "lsfg-vk: Loaded configuration for " << name << ":\n";
        if (!conf.dll.empty()) std::cerr << "  Using DLL from: " << conf.dll << '\n';
        std::cerr << "  Multiplier: " << conf.multiplier << '\n';
        std::cerr << "  Flow Scale: " << conf.flowScale << '\n';
        std::cerr << "  Performance Mode: " << (conf.performance ? "Enabled" : "Disabled") << '\n';
        std::cerr << "  HDR Mode: " << (conf.hdr ? "Enabled" : "Disabled") << '\n';
    }
}
