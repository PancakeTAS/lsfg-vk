#include "config/config.hpp"
#include "extract/extract.hpp"
#include "utils/benchmark.hpp"
#include "utils/utils.hpp"

#include <exception>
#include <stdexcept>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>

namespace {
    __attribute__((constructor)) void lsfgvk_init() {
        // read configuration
        const std::string file = Utils::getConfigFile();
        try {
            Config::loadAndWatchConfig(file);
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occured while trying to parse the configuration, exiting:\n";
            std::cerr << "- " << e.what() << '\n';
            exit(0);
        }

        const std::string name = Utils::getProcessName();
        try {
            Config::activeConf = Config::getConfig(name);
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: The configuration for " << name << " is invalid, exiting:\n";
            std::cerr << e.what() << '\n';
            exit(0);
        }

        // exit silently if not enabled
        auto& conf = Config::activeConf;
        if (!conf.enable && name != "benchmark")
            return;

        // print config
        std::cerr << "lsfg-vk: Loaded configuration for " << name << ":\n";
        if (!conf.dll.empty()) std::cerr << "  Using DLL from: " << conf.dll << '\n';
        std::cerr << "  Multiplier: " << conf.multiplier << '\n';
        std::cerr << "  Flow Scale: " << conf.flowScale << '\n';
        std::cerr << "  Performance Mode: " << (conf.performance ? "Enabled" : "Disabled") << '\n';
        std::cerr << "  HDR Mode: " << (conf.hdr ? "Enabled" : "Disabled") << '\n';

        // load shaders
        try {
            Extract::extractShaders();
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occurred while trying to extract the shaders, exiting:\n";
            std::cerr << "- " << e.what() << '\n';
            exit(0);
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
        }

        std::thread benchmark([width, height]() {
            try {
                Benchmark::run(width, height);
            } catch (const std::exception& e) {
                std::cerr << "lsfg-vk: An error occurred during the benchmark:\n";
                std::cerr << "- " << e.what() << '\n';
            }
        });
        benchmark.detach();
        conf.enable = false;
    }
}
