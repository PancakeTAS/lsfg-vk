#include "lsfg-vk-backend/lsfgvk.hpp"
#include "lsfg-vk-common/vulkan/buffer.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <bits/time.h>
#include <time.h> // NOLINT (thanks clang-tidy)

#include <vulkan/vulkan_core.h>

const VkExtent2D EXTENT = { 1920, 1080 };

namespace {
    /// returns the current time in microseconds
    uint64_t get_current_time_us() {
        struct timespec ts{};
        clock_gettime(CLOCK_MONOTONIC, &ts);

        return static_cast<uint64_t>(ts.tv_sec) * 1000000UL
            + static_cast<uint64_t>(ts.tv_nsec) / 1000UL;
    }
    /// uploads an image from a dds file
    void upload_image(
        const vk::Vulkan& vk,
        const vk::Image& image,
        const std::string& path
    ) {
        // read image bytecode
        std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw std::runtime_error("ifstream::ifstream() failed");

        std::streamsize size = file.tellg();
        size -= 124 + 4; // dds header and magic bytes

        std::vector<char> code(static_cast<size_t>(size));
        file.seekg(124 + 4, std::ios::beg);
        if (!file.read(code.data(), size))
            throw std::runtime_error("ifstream::read() failed");

        file.close();

        // upload to image
        const vk::Buffer stagingbuf{vk, code.data(), code.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT};

        const vk::CommandBuffer cmdbuf{vk};
        cmdbuf.copyBufferToImage(stagingbuf, image);

        const vk::TimelineSemaphore sema{vk, 0};
        cmdbuf.submit(vk, sema, 1, sema, 2);

        sema.signal(vk, 1);
        if (!sema.wait(vk, 2))
            throw std::runtime_error("image upload failed");
    }
}

int main() {
    const uint64_t time_us = get_current_time_us();

    const vk::Vulkan vk{
        "lsfg-vk-debug", vk::version{1, 1, 0},
        "lsfg-vk-debug-engine", vk::version{1, 0, 0},
        [](const std::vector<VkPhysicalDevice>& devices) {
            return devices.front();
        }
    };

    std::pair<int, int> srcfds{};
    const vk::Image frame_0{vk,
        EXTENT, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        std::nullopt,
        &srcfds.first
    };
    const vk::Image frame_1{vk,
        EXTENT, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        std::nullopt,
        &srcfds.second
    };

    std::vector<vk::Image> destimgs{};
    std::vector<int> destfds{};
    for (size_t i = 0; i < 1; i++) {
        int fd{};
        destimgs.emplace_back(vk,
            EXTENT, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            std::nullopt,
            &fd
        );
        destfds.push_back(fd);
    }

    int syncfd{};
    const vk::TimelineSemaphore sync{vk, 0, std::nullopt, &syncfd};

    const uint64_t init_done_us = get_current_time_us();
    std::cerr << "vulkan initialized in "
        << (init_done_us - time_us) << "us\n";

    // initialize lsfg-vk
    lsfgvk::Instance lsfgvk{
        [](const std::string&) {
            return true;
        },
        "/home/pancake/.steam/steam/steamapps/common/Lossless Scaling/Lossless.dll",
        true
    };
    lsfgvk::Context& lsfgvk_ctx = lsfgvk.openContext(
        srcfds, destfds,
        syncfd, EXTENT.width, EXTENT.height,
        false, 1.0F / 0.5F, true
    );

    const uint64_t lsfg_init_done_us = get_current_time_us();
    std::cerr << "lsfg-vk initialized in "
        << (lsfg_init_done_us - init_done_us) << "us\n";

    // render destination images
    size_t idx{1};
    for (size_t j = 0; j < 3; j++) {
        try {
            upload_image(vk,
                j % 2 == 0 ? frame_0 : frame_1,
                "s" + std::to_string(j + 1) + ".dds"
            );
        } catch (const std::exception& e) {
            std::cerr << "failed to upload image: " << e.what() << "\n";
            return EXIT_FAILURE;
        }

        sync.signal(vk, idx++);
        lsfgvk.scheduleFrames(lsfgvk_ctx);

        for (size_t i = 0; i < destimgs.size(); i++) {
            auto success = sync.wait(vk, idx++);
            if (!success) {
                std::cerr << "failed to wait for frame " << j << ":" << i << "\n";
                return EXIT_FAILURE;
            }

            const uint64_t frame_done_us = get_current_time_us();
            std::cerr << "frame " << j << ":" << i << " done after "
                << (frame_done_us - lsfg_init_done_us) << "us\n";
        }
    }

    // deinitialize lsfg-vk
    lsfgvk.closeContext(lsfgvk_ctx);
    return EXIT_SUCCESS; // let the vulkan objects go out of scope
}
