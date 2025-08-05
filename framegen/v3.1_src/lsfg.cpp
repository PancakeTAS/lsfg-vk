#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "lsfg_3_1.hpp"
#include "v3_1/context.hpp"
#include "core/commandpool.hpp"
#include "core/descriptorpool.hpp"
#include "core/instance.hpp"
#include "pool/shaderpool.hpp"
#include "common/exception.hpp"
#include "common/utils.hpp"

#include <renderdoc_app.h>
#include <dlfcn.h>

#include <cstdint>
#include <optional>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace LSFG;
using namespace LSFG_3_1;

namespace {
    std::optional<Core::Instance> instance;
    std::optional<Vulkan> device;
    std::unordered_map<int32_t, Context> contexts;

    std::optional<RENDERDOC_API_1_6_0*> renderdoc;
}

void LSFG_3_1::initialize(uint64_t deviceUUID,
        bool isHdr, float flowScale, uint64_t generationCount,
        bool forceDisableFp16,
        const std::function<std::vector<uint8_t>(const std::string&, bool)>& loader) {
    if (instance.has_value() || device.has_value())
        return;

    instance.emplace();
    device.emplace(Vulkan {
        .device{*instance, deviceUUID, forceDisableFp16},
        .generationCount = generationCount,
        .flowScale = flowScale,
        .isHdr = isHdr
    });
    contexts = std::unordered_map<int32_t, Context>();

    device->commandPool = Core::CommandPool(device->device);
    device->descriptorPool = Core::DescriptorPool(device->device);

    device->resources = Pool::ResourcePool(device->isHdr, device->flowScale);
    device->shaders = Pool::ShaderPool(loader, device->device.getFP16Support());

    std::srand(static_cast<uint32_t>(std::time(nullptr)));
}

void LSFG_3_1::initializeRenderDoc() {
    if (renderdoc.has_value())
        return;

    if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
        auto rdocGetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod, "RENDERDOC_GetAPI"));
        RENDERDOC_API_1_6_0* rdoc{};
        rdocGetAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&rdoc));

        renderdoc.emplace(rdoc);
    }

    if (!renderdoc.has_value()) {
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "RenderDoc API not found");
    }
}

int32_t LSFG_3_1::createContext(
        int in0, int in1, const std::vector<int>& outN,
        VkExtent2D extent, VkFormat format) {
    if (!instance.has_value() || !device.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    const int32_t id = std::rand();
    contexts.emplace(id, Context(*device, in0, in1, outN, extent, format));
    return id;
}

void LSFG_3_1::presentContext(int32_t id, int inSem, const std::vector<int>& outSem) {
    if (!instance.has_value() || !device.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    auto it = contexts.find(id);
    if (it == contexts.end())
        throw LSFG::vulkan_error(VK_ERROR_UNKNOWN, "Context not found");

    if (renderdoc.has_value())
        (*renderdoc)->StartFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(instance->handle()), nullptr);

    it->second.present(*device, inSem, outSem);

    if (renderdoc.has_value()) {
        vkDeviceWaitIdle(device->device.handle());
        (*renderdoc)->EndFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(instance->handle()), nullptr);
    }
}

void LSFG_3_1::deleteContext(int32_t id) {
    if (!instance.has_value() || !device.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    auto it = contexts.find(id);
    if (it == contexts.end())
        throw LSFG::vulkan_error(VK_ERROR_DEVICE_LOST, "No such context");

    vkDeviceWaitIdle(device->device.handle());
    contexts.erase(it);
}

void LSFG_3_1::finalize() {
    if (!instance.has_value() || !device.has_value())
        return;

    vkDeviceWaitIdle(device->device.handle());
    contexts.clear();
    device.reset();
    instance.reset();
}
