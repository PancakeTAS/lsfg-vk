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
    [[gnu::constructor]]
    [[gnu::visibility("default")]]
    void lsfgvk_init() {
        std::cerr << std::unitbuf;

        // read configuration
        try {
            Config::updateConfig(Utils::getConfigFile(), Utils::getProcessName());
        } catch (const std::exception& e) {
            std::cerr << "lsfg-vk: An error occured while trying to parse the configuration, IGNORING:\n";
            std::cerr << "- " << e.what() << '\n';
            return;
        }

        // exit silently if not enabled
        if (!Config::currentConf.has_value())
            return;

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
    }
}
