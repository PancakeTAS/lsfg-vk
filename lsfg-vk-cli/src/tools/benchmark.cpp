/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "benchmark.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/paths.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"
#include "lsfg-vk/lsfgvk.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <time.h>
#include <bits/time.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk::cli;
using namespace lsfgvk::cli::benchmark;

namespace {
    // Get current time in milliseconds
    uint64_t ms() {
        struct timespec ts{};
        clock_gettime(CLOCK_MONOTONIC, &ts);

        return static_cast<uint64_t>(ts.tv_sec) * 1000ULL +
            static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;
    }
}

int benchmark::run(const Options& opts) {
    try {
        // Parse options
        if (opts.flow < 0.25F || opts.flow > 1.0F)
            throw ls::error("Flow scale must be between 0.25 and 1.0");
        if (opts.multiplier < 2)
            throw ls::error("Multiplier must be 2 or greater");
        if (opts.width <= 0 || opts.height <= 0)
            throw ls::error("Width and height must be positive integers");
        if (opts.duration <= 0)
            throw ls::error("Duration must be a positive integer");
        const VkExtent2D extent{
            static_cast<uint32_t>(opts.width),
            static_cast<uint32_t>(opts.height)
        };

        // Create instance
        std::string gpu_name{};

        const vk::Vulkan vk{
            "lsfg-vk-debug", vk::version{2, 0, 0},
            "lsfg-vk-debug", vk::version{2, 0, 0},
            [opts, gpu_name = &gpu_name](const vk::VulkanInstanceFuncs fi,
                    const std::vector<VkPhysicalDevice>& devices) {
                for (const VkPhysicalDevice& device : devices) {
                    VkPhysicalDeviceProperties2 props{
                        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
                    };
                    fi.GetPhysicalDeviceProperties2(device, &props);

                    auto& properties = props.properties;
                    std::array<char, 256> devname = std::to_array(properties.deviceName);
                    devname.at(255) = '\0'; // Ensure null-termination

                    if (!opts.gpu || std::string(devname.data()) == *opts.gpu) {
                        *gpu_name = std::string(devname.data());
                        return device;
                    }
                }

                throw ls::error("Failed to find specified GPU: " + *opts.gpu);
            }
        };

        // Initialize backend
        std::string dll{};
        if (opts.dll.has_value())
            dll = *opts.dll;
        else
            dll = ls::findShaderDll();

        const lsfgvk::Instance lsfgvk{
            gpu_name,
            dll,
            opts.allow_fp16
        };
        lsfgvk::Context lsfgvk_ctx{
            lsfgvk,
            extent.width, extent.height,
            opts.flow, opts.performance_mode
        };

        // Import resources
        const auto fds{lsfgvk_ctx.exportFds()};

        const vk::Image source{vk,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            fds.sourceFd, std::nullopt, 2
        };
        const vk::Image destination{vk,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            fds.destinationFd
        };
        const vk::TimelineSemaphore sync{vk,
            0,
            fds.syncFd
        };

        // Run the benchmark
        const uint32_t total{static_cast<uint32_t>(opts.multiplier) - 1U};

        size_t iterations{0};
        size_t generated_frames{0};
        size_t total_frames{0};
        size_t idx{1};

        uint64_t print_time = ms() + 1000ULL;
        const uint64_t end_time = ms() + static_cast<uint64_t>(opts.duration) * 1000ULL;
        while (ms() < end_time) {
            lsfgvk_ctx.dispatch(total);

            for (size_t i = 0; i < total; i++) {
                sync.signal(vk, idx++);

                auto success = sync.wait(vk, idx++);
                if (!success)
                    throw ls::error("failed to wait for frame");

                total_frames++;
                generated_frames++;
            }

            total_frames++;
            iterations++;

            if (ms() >= print_time) {
                print_time += 1000ULL;
                std::cerr << "." << std::flush;
            }
        }

        // Output results
        std::cerr << (opts.duration < 40 ? "\r" : "\n");
        std::cerr << "Benchmark results (ran for " << opts.duration << " seconds):\n";
        std::cerr << "  Iterations:       " << iterations << "\n";
        std::cerr << "  Generated frames: " << generated_frames << "\n";
        std::cerr << "  Total frames:     " << total_frames << "\n";
        const auto time = static_cast<double>(opts.duration);
        const double fps_generated = static_cast<double>(generated_frames) / time;
        const double fps_total = static_cast<double>(total_frames) / time;
        std::cerr << std::setprecision(2) << std::fixed;
        std::cerr << "  FPS (generated):  " << fps_generated << "fps\n";
        std::cerr << "  FPS (total):      " << fps_total << "fps\n";

        // Wait for idle
        lsfgvk_ctx.idle();

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
