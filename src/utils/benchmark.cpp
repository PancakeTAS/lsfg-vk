#include "utils/benchmark.hpp"
#include "config/config.hpp"
#include "extract/extract.hpp"
#include "extract/trans.hpp"

#include <vulkan/vulkan_core.h>
#include <lsfg_3_1.hpp>
#include <lsfg_3_1p.hpp>
#include <unistd.h>

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

using namespace Benchmark;

void Benchmark::run(uint32_t width, uint32_t height) {
    const auto& conf = Config::activeConf;

    auto* lsfgInitialize = LSFG_3_1::initialize;
    auto* lsfgCreateContext = LSFG_3_1::createContext;
    auto* lsfgPresentContext = LSFG_3_1::presentContext;
    if (conf.performance) {
        lsfgInitialize = LSFG_3_1P::initialize;
        lsfgCreateContext = LSFG_3_1P::createContext;
        lsfgPresentContext = LSFG_3_1P::presentContext;
    }

    // create the benchmark context
    const char* lsfgDeviceUUID = std::getenv("LSFG_DEVICE_UUID");
    const uint64_t deviceUUID = lsfgDeviceUUID
        ? std::stoull(std::string(lsfgDeviceUUID), nullptr, 16) : 0x1463ABAC;

    setenv("DISABLE_LSFG", "1", 1); // NOLINT

    Extract::extractShaders();
    lsfgInitialize(
        deviceUUID, // some magic number if not given
        conf.hdr, 1.0F / conf.flowScale, conf.multiplier - 1,
        [](const std::string& name) -> std::vector<uint8_t> {
            auto dxbc = Extract::getShader(name);
            auto spirv = Extract::translateShader(dxbc);
            return spirv;
        }
    );
    const int32_t ctx = lsfgCreateContext(-1, -1, {},
        { .width = width, .height = height },
        conf.hdr ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM
    );

    unsetenv("DISABLE_LSFG"); // NOLINT

    // run the benchmark (run 8*n + 1 so the fences are waited on)
    const auto now = std::chrono::high_resolution_clock::now();
    const uint64_t iterations = 8 * 500UL;

    std::cerr << "lsfg-vk: Benchmark started, running " << iterations << " iterations...\n";
    for (uint64_t count = 0; count < iterations + 1; count++) {
        lsfgPresentContext(ctx, -1, {});

        if (count % 50 == 0 && count > 0)
            std::cerr << "lsfg-vk: "
                            << std::setprecision(2) << std::fixed
                            << static_cast<float>(count) / static_cast<float>(iterations) * 100.0F
                        << "% done (" << count + 1 << "/" << iterations << ")\r";
    }
    const auto then = std::chrono::high_resolution_clock::now();

    // print results
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(then - now).count();

    const auto perIteration = static_cast<float>(ms) / static_cast<float>(iterations);

    const uint64_t totalGen = (conf.multiplier - 1) * iterations;
    const auto genFps = static_cast<float>(totalGen) / (static_cast<float>(ms) / 1000.0F);

    const uint64_t totalFrames = iterations * conf.multiplier;
    const auto totalFps = static_cast<float>(totalFrames) / (static_cast<float>(ms) / 1000.0F);

    std::cerr << "lsfg-vk: Benchmark completed in " << ms << " ms\n";
    std::cerr << "  Time taken per real frame: "
              << std::setprecision(2) << std::fixed << perIteration << " ms\n";
    std::cerr << "  Generated " << totalGen << " frames in total at "
              << std::setprecision(2) << std::fixed << genFps << " FPS\n";
    std::cerr << "  Total of " << totalFrames << " frames presented at "
              << std::setprecision(2) << std::fixed << totalFps << " FPS\n";

    // sleep for a second, then exit
    std::this_thread::sleep_for(std::chrono::seconds(1));
    _exit(0);
}
