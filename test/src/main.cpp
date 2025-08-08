#include "core/device.hpp"
#include "core/instance.hpp"
#include "extract/extract.hpp"
#include "mini/image.hpp"
#include "utils.hpp"

#include <vulkan/vulkan_core.h>
#include <renderdoc_app.h>
#include <dlfcn.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

using namespace LSFG;

// test configuration

const VkExtent2D SRC_EXTENT = { 2560 , 1440 };
const VkFormat SRC_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
const std::array<std::string, 2> SRC_FILES = {
    "test/f0.dds",
    "test/f1.dds"
};

const size_t MULTIPLIER = 4;
const bool IS_HDR = false;
const float FLOW_SCALE = 1.0F;
#define PERFORMANCE_MODE true


// test configuration end

#ifdef PERFORMANCE_MODE
#include "lsfg_3_1p.hpp"
using namespace LSFG_3_1P;
#else
#include "lsfg_3_1.hpp"
using namespace LSFG_3_1;
#endif

namespace {
    RENDERDOC_API_1_6_0* rdoc{};

    /// Attempt to load the RenderDoc API.
    void setup_renderdoc() {
        if(void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
            auto rdocGetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod, "RENDERDOC_GetAPI"));
            rdocGetAPI(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&rdoc));
        }
    }

    /// Create images for frame generation
    void create_images(const Core::Device& device,
            std::array<int, 2>& fds,
            std::vector<int>& outFds, std::vector<Mini::Image>& out_n) {
        const Mini::Image frame_0{device.handle(), device.getPhysicalDevice(),
            SRC_EXTENT, SRC_FORMAT,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            &fds.at(0)
        };
        const Mini::Image frame_1{device.handle(), device.getPhysicalDevice(),
            SRC_EXTENT, SRC_FORMAT,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            &fds.at(1)
        };

        for (size_t i = 0; i < (MULTIPLIER - 1); i++)
            out_n.at(i) = Mini::Image{device.handle(), device.getPhysicalDevice(),
                SRC_EXTENT, SRC_FORMAT,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                &outFds.at(i)
            };
    }

    /// Create the LSFG context.
    int32_t create_lsfg(const std::array<int, 2>& fds, const std::vector<int>& outFds) {
        Extract::extractShaders();
        initialize(
            0x1463ABAC,
            IS_HDR, FLOW_SCALE, MULTIPLIER - 3,
            Extract::getShader
        );
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
    std::vector<Mini::Image> out_n(MULTIPLIER - 1);
    int32_t lsfg_id{};
}

int main() {
    // initialize host Vulkan
    const Core::Instance instance{};
    const Core::Device device{instance, 0x1463ABAC};

    // setup test
    setup_renderdoc();
    create_images(device, fds, outFds, out_n);
    lsfg_id = create_lsfg(fds, outFds);

    // run
    for (size_t fc = 0; fc < SRC_FILES.size(); fc++) {
    }

    // destroy test
    delete_lsfg(lsfg_id);

    return 0;
}
