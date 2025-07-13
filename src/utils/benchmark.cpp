#include "utils/benchmark.hpp"
#include "extract/extract.hpp"
#include "extract/trans.hpp"
#include "utils/log.hpp"

#include <vulkan/vulkan_core.h>
#include <lsfg_3_1.hpp>
#include <lsfg_3_1p.hpp>

#include <cstdint>
#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Benchmark;

void Benchmark::run() {
    // fetch benchmark parameters
    const char* lsfgFlowScale = std::getenv("LSFG_FLOW_SCALE");
    const char* lsfgHdr = std::getenv("LSFG_HDR");
    const char* lsfgMultiplier = std::getenv("LSFG_MULTIPLIER");
    const char* lsfgExtentWidth = std::getenv("LSFG_EXTENT_WIDTH");
    const char* lsfgExtentHeight = std::getenv("LSFG_EXTENT_HEIGHT");
    const char* lsfgPerfMode = std::getenv("LSFG_PERF_MODE");

    const float flowScale = lsfgFlowScale
        ? std::stof(lsfgFlowScale) : 1.0F;
    const bool isHdr = lsfgHdr
        ? *lsfgHdr == '1' : false;
    const uint64_t multiplier = lsfgMultiplier
        ? std::stoull(std::string(lsfgMultiplier)) : 2;
    const uint32_t width = lsfgExtentWidth
        ? static_cast<uint32_t>(std::stoul(lsfgExtentWidth)) : 1920;
    const uint32_t height = lsfgExtentHeight
        ? static_cast<uint32_t>(std::stoul(lsfgExtentHeight)) : 1080;
    const bool perfMode = lsfgPerfMode
        ? *lsfgPerfMode == '1' : false;

    auto* lsfgInitialize = LSFG_3_1::initialize;
    auto* lsfgCreateContext = LSFG_3_1::createContext;
    auto* lsfgPresentContext = LSFG_3_1::presentContext;
    if (perfMode) {
        lsfgInitialize = LSFG_3_1P::initialize;
        lsfgCreateContext = LSFG_3_1P::createContext;
        lsfgPresentContext = LSFG_3_1P::presentContext;
    }

    Log::info("bench", "Running {}x benchmark with {}x{} extent and flow scale of {} {} HDR",
        multiplier, width, height, flowScale, isHdr ? "with" : "without");

    // create the benchmark context
    const char* lsfgDeviceUUID = std::getenv("LSFG_DEVICE_UUID");
    const uint64_t deviceUUID = lsfgDeviceUUID
        ? std::stoull(std::string(lsfgDeviceUUID), nullptr, 16) : 0x1463ABAC;

    Extract::extractShaders();
    lsfgInitialize(
        deviceUUID, // some magic number if not given
        isHdr, 1.0F / flowScale, multiplier - 1,
        [](const std::string& name) -> std::vector<uint8_t> {
            auto dxbc = Extract::getShader(name);
            auto spirv = Extract::translateShader(dxbc);
            return spirv;
        }
    );
    const int32_t ctx = lsfgCreateContext(-1, -1, {},
        { .width = width, .height = height },
        isHdr ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM
    );

    Log::info("bench", "Benchmark context created, ready to run");

    // run the benchmark (run 8*n + 1 so the fences are waited on)
    const auto now = std::chrono::high_resolution_clock::now();
    const uint64_t iterations = (8 * 500) + 1;
    for (uint64_t count = 0; count < iterations; count++) {
        lsfgPresentContext(ctx, -1, {});

        if (count % 500 == 0)
            Log::info("bench", "{:.2f}% done ({}/{})",
                static_cast<float>(count) / static_cast<float>(iterations) * 100.0F,
                count + 1, iterations);
    }
    const auto then = std::chrono::high_resolution_clock::now();

    // print results
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(then - now).count();

    const auto perIteration = static_cast<float>(ms) / static_cast<float>(iterations);

    const uint64_t totalGen = (multiplier - 1) * iterations;
    const auto genFps = static_cast<float>(totalGen) / (static_cast<float>(ms) / 1000.0F);

    const uint64_t totalFrames = iterations * multiplier;
    const auto totalFps = static_cast<float>(totalFrames) / (static_cast<float>(ms) / 1000.0F);

    Log::info("bench", "Benchmark completed in {} ms", ms);
    Log::info("bench", "Time per iteration: {:.2f} ms", perIteration);
    Log::info("bench", "Generation FPS: {:.2f}", genFps);
    Log::info("bench", "Final FPS: {:.2f}", totalFps);
    Log::info("bench", "Benchmark finished, exiting");
}
