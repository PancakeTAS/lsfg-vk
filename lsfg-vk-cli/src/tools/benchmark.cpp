/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "benchmark.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/paths.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

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
#include <utility>
#include <vector>

#include <time.h>
#include <bits/time.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk::cli;
using namespace lsfgvk::cli::benchmark;

namespace {
    // get current time in milliseconds
    uint64_t ms() {
        struct timespec ts{};
        clock_gettime(CLOCK_MONOTONIC, &ts);

        return static_cast<uint64_t>(ts.tv_sec) * 1000ULL +
            static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;
    }
}

int benchmark::run(const Options& opts) {
    try {
        // parse options
        if (opts.flow < 0.25F || opts.flow > 1.0F)
            throw ls::error("flow scale must be between 0.25 and 1.0");
        if (opts.multiplier < 2)
            throw ls::error("multiplier must be 2 or greater");
        if (opts.width <= 0 || opts.height <= 0)
            throw ls::error("width and height must be positive integers");
        if (opts.duration <= 0)
            throw ls::error("duration must be a positive integer");
        const VkExtent2D extent{
            static_cast<uint32_t>(opts.width),
            static_cast<uint32_t>(opts.height)
        };

        // create instance
        const vk::Vulkan vk{
            "lsfg-vk-debug", vk::version{2, 0, 0},
            "lsfg-vk-debug-engine", vk::version{2, 0, 0},
            [opts](const vk::VulkanInstanceFuncs fi,
                    const std::vector<VkPhysicalDevice>& devices) {
                if (!opts.gpu.has_value())
                    return devices.front();

                for (const VkPhysicalDevice& device : devices) {
                    VkPhysicalDeviceProperties2 props{
                        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
                    };
                    fi.GetPhysicalDeviceProperties2(device, &props);

                    auto& properties = props.properties;
                    std::array<char, 256> devname = std::to_array(properties.deviceName);
                    devname.at(255) = '\0'; // ensure null-termination

                    if (std::string(devname.data()) == *opts.gpu)
                        return device;
                }

                throw ls::error("failed to find specified GPU: " + *opts.gpu);
            }
        };

        std::pair<int, int> srcfds{};
        const vk::Image frame_0{vk,
            extent, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            std::nullopt, &srcfds.first};
        const vk::Image frame_1{vk,
            extent, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            std::nullopt, &srcfds.second};

        std::vector<vk::Image> destimgs{};
        std::vector<int> destfds{};
        for (int i = 0; i < (opts.multiplier - 1); i++) {
            int fd{};
            destimgs.emplace_back(vk,
                extent, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                std::nullopt,
                &fd
            );
            destfds.push_back(fd);
        }

        int syncfd{};
        const vk::TimelineSemaphore sync{vk, 0, std::nullopt, &syncfd};

        // initialize backend
        std::string dll{};
        if (opts.dll.has_value())
            dll = *opts.dll;
        else
            dll = ls::findShaderDll();

        lsfgvk::backend::Instance lsfgvk{
            [opts](
                const std::string& gpu_name,
                std::pair<const std::string&, const std::string&>,
                const std::optional<std::string>&
            ) {
                return opts.gpu.value_or(gpu_name) == gpu_name;
            },
            dll, opts.allow_fp16
        };
        lsfgvk::backend::Context& lsfgvk_ctx = lsfgvk.openContext(
            srcfds, destfds,
            syncfd, extent.width, extent.height,
            false, 1.0F / opts.flow, opts.performance_mode
        );

        // run the benchmark
        size_t iterations{0};
        size_t generated_frames{0};
        size_t total_frames{1};

        uint64_t print_time = ms() + 1000ULL;
        const uint64_t end_time = ms() + static_cast<uint64_t>(opts.duration) * 1000ULL;
        while (ms() < end_time) {
            sync.signal(vk, total_frames++);
            lsfgvk.scheduleFrames(lsfgvk_ctx);

            for (size_t i = 0; i < destimgs.size(); i++) {
                auto success = sync.wait(vk, total_frames++);
                if (!success)
                    throw ls::error("failed to wait for frame");

                generated_frames++;
            }

            iterations++;

            if (ms() >= print_time) {
                print_time += 1000ULL;
                std::cerr << "." << std::flush;
            }
        }

        // output results

        std::cerr << (opts.duration < 40 ? "\r" : "\n");
        std::cerr << "benchmark results (ran for " << opts.duration << " seconds):\n";
        std::cerr << "  iterations:       " << iterations << "\n";
        std::cerr << "  generated frames: " << generated_frames << "\n";
        std::cerr << "  total frames:     " << total_frames << "\n";
        const auto time = static_cast<double>(opts.duration);
        const double fps_generated = static_cast<double>(generated_frames) / time;
        const double fps_total = static_cast<double>(total_frames) / time;
        std::cerr << std::setprecision(2) << std::fixed;
        std::cerr << "  fps (generated):  " << fps_generated << "fps\n";
        std::cerr << "  fps (total):      " << fps_total << "fps\n";

        // deinitialize lsfg-vk
        lsfgvk.closeContext(lsfgvk_ctx);
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
