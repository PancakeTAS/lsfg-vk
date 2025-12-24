/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "debug.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/helpers/paths.hpp"
#include "lsfg-vk-common/vulkan/buffer.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

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
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace lsfgvk::cli;
using namespace lsfgvk::cli::debug;

namespace {
    /// uploads an image from a dds file
    void upload_image(const vk::Vulkan& vk,
            const vk::Image& image, const std::string& path) {
        // read image bytecode
        std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw ls::error("ifstream::ifstream() failed");

        std::streamsize size = file.tellg();
        size -= 124 + 4; // dds header and magic bytes

        std::vector<char> code(static_cast<size_t>(size));
        file.seekg(124 + 4, std::ios::beg);
        if (!file.read(code.data(), size))
            throw ls::error("ifstream::read() failed");

        file.close();

        // upload to image
        const vk::Buffer stagingbuf{vk, code.data(), code.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT};

        const vk::CommandBuffer cmdbuf{vk};
        cmdbuf.begin(vk);
        cmdbuf.copyBufferToImage(vk, stagingbuf, image);
        cmdbuf.end(vk);

        const vk::TimelineSemaphore sema{vk, 0};
        cmdbuf.submit(vk);
    }
}

int debug::run(const Options& opts) {
    try {
        // parse options
        if (opts.flow < 0.25F || opts.flow > 1.0F)
            throw ls::error("flow scale must be between 0.25 and 1.0");
        if (opts.multiplier < 2)
            throw ls::error("multiplier must be 2 or greater");
        if (opts.width <= 0 || opts.height <= 0)
            throw ls::error("width and height must be positive integers");
        const VkExtent2D extent{
            static_cast<uint32_t>(opts.width),
            static_cast<uint32_t>(opts.height)
        };
        if (!std::filesystem::exists(opts.path))
            throw ls::error("debug path does not exist: " + opts.path.string());
        std::vector<std::filesystem::path> paths{};
        for (const auto& entry : std::filesystem::directory_iterator(opts.path))
            paths.push_back(entry.path());
        std::ranges::sort(paths, [](const std::filesystem::path& a, const std::filesystem::path& b) {
            auto fa = a.filename().string();
            auto fb = b.filename().string();

            auto norm_a = fa.find_first_of('.');
            if (norm_a == std::string::npos)
                throw ls::error("invalid debug file name: " + fa);
            auto norm_b = fb.find_first_of('.');
            if (norm_b == std::string::npos)
                throw ls::error("invalid debug file name: " + fb);

            return std::stoi(fa.substr(0, norm_a)) < std::stoi(fb.substr(0, norm_b));
        });

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
                    devname[255] = '\0'; // ensure null-termination

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

        // render destination images
        size_t idx{1};
        for (size_t j = 0; j < paths.size(); j++) {
            upload_image(vk,
                j % 2 == 0 ? frame_0 : frame_1,
                paths.at(j).string()
            );

            sync.signal(vk, idx++);
            lsfgvk.scheduleFrames(lsfgvk_ctx);

            for (size_t i = 0; i < destimgs.size(); i++) {
                auto success = sync.wait(vk, idx++);
                if (!success)
                    throw ls::error("failed to wait for frame");
            }
        }

        // deinitialize lsfg-vk
        lsfgvk.closeContext(lsfgvk_ctx);
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
