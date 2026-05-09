/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "debug.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/paths.hpp"
#include "lsfg-vk-common/vulkan/buffer.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#define LSFGVK_PRIV
#include "lsfg-vk/lsfgvk.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <dlfcn.h>
#include <renderdoc_app.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk::cli;
using namespace lsfgvk::cli::debug;

namespace {
    /// Upload an image from a DDS file
    void uploadDDS(const vk::Vulkan& vk,
            const vk::Image& image,
            const std::string& path,
            uint32_t layer
    ) {
        // Read image data
        std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw ls::error("ifstream::ifstream() failed");

        std::streamsize size = static_cast<std::streamsize>(file.tellg());
        size -= 124 + 4; // DDS header and magic bytes

        std::vector<char> code(static_cast<size_t>(size));
        file.seekg(124 + 4, std::ios::beg);
        if (!file.read(code.data(), size))
            throw ls::error("ifstream::read() failed");

        file.close();

        // Upload to image
        const vk::Buffer stagingbuf{vk, code.data(), code.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT};

        const vk::CommandBuffer cmdbuf{vk};
        cmdbuf.begin(vk);
        cmdbuf.copyBufferToImage(vk, stagingbuf, image, layer);
        cmdbuf.end(vk);

        const vk::TimelineSemaphore sema{vk, 0};
        cmdbuf.submit(vk);
    }
}

int debug::run(const Options& opts) {
    try {
        // Parse options
        if (opts.flow < 0.25F || opts.flow > 1.0F)
            throw ls::error("Flow scale must be between 0.25 and 1.0");
        if (opts.multiplier < 2)
            throw ls::error("Multiplier must be 2 or greater");
        if (opts.width <= 0 || opts.height <= 0)
            throw ls::error("Width and height must be positive integers");
        const VkExtent2D extent{
            static_cast<uint32_t>(opts.width),
            static_cast<uint32_t>(opts.height)
        };
        if (!std::filesystem::exists(opts.path))
            throw ls::error("Debug path does not exist: " + opts.path.string());
        std::vector<std::filesystem::path> paths{};
        for (const auto& entry : std::filesystem::directory_iterator(opts.path))
            paths.push_back(entry.path());
        std::ranges::sort(paths, [](const std::filesystem::path& a, const std::filesystem::path& b) {
            auto fa = a.filename().string();
            auto fb = b.filename().string();

            auto norm_a = fa.find_first_of('.');
            if (norm_a == std::string::npos)
                throw ls::error("Invalid debug file name: " + fa);
            auto norm_b = fb.find_first_of('.');
            if (norm_b == std::string::npos)
                throw ls::error("Invalid debug file name: " + fb);

            return std::stoi(fa.substr(0, norm_a)) < std::stoi(fb.substr(0, norm_b));
        });

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

        // Try to open RenderDoc
        RENDERDOC_API_1_6_0* rdoc_api{nullptr};
        RENDERDOC_DevicePointer rdoc_device{nullptr};
        if (void* module = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
            void* func{dlsym(module, "RENDERDOC_GetAPI")};

            auto* GetAPI{reinterpret_cast<pRENDERDOC_GetAPI>(func)}; // NOLINT (unsafe cast)
            GetAPI(
                eRENDERDOC_API_Version_1_0_0,
                reinterpret_cast<void**>(&rdoc_api) // NOLINT (unsafe cast)
            );

            rdoc_device = RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(lsfgvk._instance());
        }

        // Render destination images
        const uint32_t total{static_cast<uint32_t>(opts.multiplier) - 1U};

        size_t idx{1};
        for (size_t j = 0; j < paths.size(); j++) {
            uploadDDS(vk, source, paths.at(j).string(), j % 2);

            if (rdoc_api) {
                rdoc_api->StartFrameCapture(rdoc_device, nullptr);
            }

            std::thread signal_thread{[&sync, &vk, &idx, total] {
                for (size_t i = 0; i < total; i++) {
                    sync.signal(vk, idx++);

                    auto success = sync.wait(vk, idx++);
                    if (!success)
                        throw ls::error("Failed to wait for frame");
                }
            }};

            lsfgvk_ctx.dispatch(total);

            if (rdoc_api) {
                lsfgvk_ctx.idle();
                rdoc_api->EndFrameCapture(rdoc_device, nullptr);
            }

            signal_thread.join();
        }

        // Wait for idle
        lsfgvk_ctx.idle();

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
