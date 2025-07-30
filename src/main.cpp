#include "config/config.hpp"
#include "extract/extract.hpp"
#include "utils/benchmark.hpp"
#include "utils/utils.hpp"

#include <unistd.h>

#include <exception>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>

namespace {
    __attribute__((constructor))
    __attribute__((visibility("default")))
    void lsfgvk_init() {
        std::cerr << std::unitbuf;

        // read configuration
        const std::string file = Utils::getConfigFile();
        try {
            Config::updateConfig(file);
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occured while trying to parse the configuration, IGNORING:\n";
            std::cerr << "- " << e.what() << '\n';
            return; // default configuration will unload
        }

        const auto name = Utils::getProcessName();
        try {
            Config::activeConf = Config::getConfig(name);
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: The configuration for " << name.second << " is invalid, IGNORING:\n";
            std::cerr << e.what() << '\n';
            return; // default configuration will unload
        }

        // exit silently if not enabled
        auto& conf = Config::activeConf;
        if (!conf.enable && name.second != "benchmark")
            return; // default configuration will unload

        // print config
        std::cerr << "lsfg-vk: Loaded configuration for " << name.second << ":\n";
        if (!conf.dll.empty()) std::cerr << "  Using DLL from: " << conf.dll << '\n';
        std::cerr << "  Multiplier: " << conf.multiplier << '\n';
        std::cerr << "  Flow Scale: " << conf.flowScale << '\n';
        std::cerr << "  Performance Mode: " << (conf.performance ? "Enabled" : "Disabled") << '\n';
        std::cerr << "  HDR Mode: " << (conf.hdr ? "Enabled" : "Disabled") << '\n';
        if (conf.e_present != 2) std::cerr << "  ! Present Mode: " << conf.e_present << '\n';

        // remove mesa var in favor of config
        unsetenv("MESA_VK_WSI_PRESENT_MODE"); // NOLINT

        // write latest file
        try {
            std::ofstream latest("/tmp/lsfg-vk_last", std::ios::trunc);
            if (!latest.is_open())
                throw std::runtime_error("Failed to open /tmp/lsfg-vk_last for writing");
            latest << "exe: " << name.first << '\n';
            latest << "comm: " << name.second << '\n';
            latest << "pid: " << getpid() << '\n';
            if (!latest.good())
                throw std::runtime_error("Failed to write to /tmp/lsfg-vk_last");
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occurred while trying to write the latest file, exiting:\n";
            std::cerr << "- " << e.what() << '\n';
            exit(EXIT_FAILURE);
        }

        // load shaders
        try {
            Extract::extractShaders();
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occurred while trying to extract the shaders, exiting:\n";
            std::cerr << "- " << e.what() << '\n';
            exit(EXIT_FAILURE);
        }
        std::cerr << "lsfg-vk: Shaders extracted successfully.\n";

        // run benchmark if requested
        const char* benchmark_flag = std::getenv("LSFG_BENCHMARK");
        if (!benchmark_flag)
            return;

        const std::string resolution(benchmark_flag);
        uint32_t width{};
        uint32_t height{};
        try {
            const size_t x = resolution.find('x');
            if (x == std::string::npos)
                throw std::runtime_error("Unable to find 'x' in benchmark string");

            const std::string width_str = resolution.substr(0, x);
            const std::string height_str = resolution.substr(x + 1);
            if (width_str.empty() || height_str.empty())
                throw std::runtime_error("Invalid resolution");

            const int32_t w = std::stoi(width_str);
            const int32_t h = std::stoi(height_str);
            if (w < 0 || h < 0)
                throw std::runtime_error("Resolution cannot be negative");

            width = static_cast<uint32_t>(w);
            height = static_cast<uint32_t>(h);
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occurred while trying to parse the resolution, exiting:\n";
            std::cerr << "- " << e.what() << '\n';
            exit(EXIT_FAILURE);
        }

        std::thread benchmark([width, height]() {
            try {
                Benchmark::run(width, height);
            } catch (const std::exception& e) {
                std::cerr << "lsfg-vk: An error occurred during the benchmark:\n";
                std::cerr << "- " << e.what() << '\n';
                exit(EXIT_FAILURE);
            }
        });
        benchmark.detach();
        conf.enable = false;
    }
}
