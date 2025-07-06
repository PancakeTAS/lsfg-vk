#include "lsfg.hpp"
#include "core/device.hpp"
#include "core/instance.hpp"
#include "context.hpp"
#include "pool/shaderpool.hpp"
#include "utils/utils.hpp"

#include <cstdlib>
#include <ctime>
#include <optional>
#include <string>
#include <unordered_map>

using namespace LSFG;

namespace {
    std::optional<Core::Instance> instance;
    std::optional<Core::Device> device;
    std::optional<Pool::ShaderPool> pool;
    std::unordered_map<int32_t, Context> contexts;
}

void LSFG::initialize() {
    if (instance.has_value() || device.has_value())
        return;

    char* dllPath = getenv("LSFG_DLL_PATH");
    std::string dllPathStr; // (absolutely beautiful code)
    if (dllPath && *dllPath != '\0') {
        dllPathStr = std::string(dllPath);
    } else {
        const char* dataDir = getenv("XDG_DATA_HOME");
        if (dataDir && *dataDir != '\0') {
            dllPathStr = std::string(dataDir) +
                "/Steam/steamapps/common/Lossless Scaling/Lossless.dll";
        } else {
            const char* homeDir = getenv("HOME");
            if (homeDir && *homeDir != '\0') {
                dllPathStr = std::string(homeDir) +
                    "/.local/share/Steam/steamapps/common/Lossless Scaling/Lossless.dll";
            } else {
                dllPathStr = "Lossless.dll";
            }
        }
    }

    instance.emplace();
    device.emplace(*instance);
    pool.emplace(dllPathStr);

    Globals::initializeGlobals(*device);

    std::srand(static_cast<uint32_t>(std::time(nullptr)));
}

int32_t LSFG::createContext(uint32_t width, uint32_t height, int in0, int in1,
        const std::vector<int>& outN) {
    if (!instance.has_value() || !device.has_value() || !pool.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    auto id = std::rand();
    contexts.emplace(id, Context(*device, *pool, width, height, in0, in1, outN));
    return id;
}

void LSFG::presentContext(int32_t id, int inSem, const std::vector<int>& outSem) {
    if (!instance.has_value() || !device.has_value() || !pool.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    auto it = contexts.find(id);
    if (it == contexts.end())
        throw LSFG::vulkan_error(VK_ERROR_DEVICE_LOST, "No such context");

    Context& context = it->second;
    context.present(*device, inSem, outSem);
}

void LSFG::deleteContext(int32_t id) {
    if (!instance.has_value() || !device.has_value() || !pool.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    auto it = contexts.find(id);
    if (it == contexts.end())
        throw LSFG::vulkan_error(VK_ERROR_DEVICE_LOST, "No such context");

    vkDeviceWaitIdle(device->handle());
    contexts.erase(it);
}

void LSFG::finalize() {
    if (!instance.has_value() || !device.has_value() || !pool.has_value())
        return;

    Globals::uninitializeGlobals();

    vkDeviceWaitIdle(device->handle());
    pool.reset();
    device.reset();
    instance.reset();
}
