#include "common/utils.hpp"
#include "core/commandpool.hpp"
#include "core/device.hpp"
#include "core/image.hpp"
#include "core/instance.hpp"
#include "extract/extract.hpp"
#include "extract/trans.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdlib>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

using namespace LSFG;

// test configuration

const VkExtent2D SRC_EXTENT = { 2560 , 1440 };
const VkFormat SRC_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
const std::array<std::string, 3> SRC_FILES = {
    "test/f0.dds",
    "test/f1.dds",
    "test/f2.dds"
};

const size_t MULTIPLIER = 3;
const bool IS_HDR = false;
const float FLOW_SCALE = 0.7F;
#define PERFORMANCE_MODE false

// test configuration end

#ifdef PERFORMANCE_MODE
#include "lsfg_3_1p.hpp"
using namespace LSFG_3_1P;
#else
#include "lsfg_3_1.hpp"
using namespace LSFG_3_1;
#endif

namespace {
    /// Create images for frame generation
    std::pair<Core::Image, Core::Image> create_images(const Core::Device& device,
            std::array<int, 2>& fds,
            std::vector<int>& outFds, std::vector<Core::Image>& out_n) {
        const Core::Image frame_0{device,
            SRC_EXTENT, SRC_FORMAT,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            &fds.at(0)
        };
        const Core::Image frame_1{device,
            SRC_EXTENT, SRC_FORMAT,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            &fds.at(1)
        };

        for (size_t i = 0; i < (MULTIPLIER - 1); i++)
            out_n.at(i) = Core::Image{device,
                SRC_EXTENT, SRC_FORMAT,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                &outFds.at(i)
            };

        return { frame_0, frame_1 };
    }

    /// Create the LSFG context.
    int32_t create_lsfg(const std::array<int, 2>& fds, const std::vector<int>& outFds) {
        Extract::extractShaders();
        initialize(
            0x1463ABAC,
            IS_HDR, 1.0F / FLOW_SCALE, MULTIPLIER - 1,
            [](const std::string& name) -> std::vector<uint8_t> {
                auto dxbc = Extract::getShader(name);
                auto spirv = Extract::translateShader(dxbc);
                return spirv;
            }
        );
        initializeRenderDoc();
        return createContext(
            fds.at(0), fds.at(1), outFds,
            SRC_EXTENT, SRC_FORMAT
        );
    }

    /// Destroy the LSFG context.
    void delete_lsfg(int32_t id) {
        deleteContext(id);
        finalize();
    }
}

namespace {
    std::array<int, 2> fds{};
    std::vector<int> outFds(MULTIPLIER - 1);
    std::pair<Core::Image, Core::Image> frames{};
    std::vector<Core::Image> out_n(MULTIPLIER - 1);
    int32_t lsfg_id{};
}

int main() {
    // initialize host Vulkan
    const Core::Instance instance{};
    const Core::Device device{instance, 0x1463ABAC};
    const Core::CommandPool commandPool{device};

    // setup test
    frames = create_images(device, fds, outFds, out_n);
    lsfg_id = create_lsfg(fds, outFds);

    Utils::clearImage(device, frames.first);
    Utils::clearImage(device, frames.second);

    // run
    for (size_t fc = 0; fc < SRC_FILES.size(); fc++) {
        if (fc % 2 == 0)
            Utils::uploadImage(device, commandPool, frames.first, SRC_FILES.at(fc));
        else
            Utils::uploadImage(device, commandPool, frames.second, SRC_FILES.at(fc));

        // run the present
        presentContext(lsfg_id, -1, {});
    }

    // destroy test
    delete_lsfg(lsfg_id);

    return 0;
}
