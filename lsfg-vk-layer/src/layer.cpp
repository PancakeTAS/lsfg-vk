#include "layer.hpp"
#include "detection.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk;
using namespace lsfgvk::layer;

Layer::Layer() : identification(identify()) {
    this->tick();

    if (!this->profile.has_value())
        return;

    std::cerr << "lsfg-vk: using profile with name '" << this->profile->name << "' ";
    switch (this->identType) {
        case IdentType::OVERRIDE:
            std::cerr << "(identified via override)\n";
            break;
        case IdentType::EXECUTABLE:
            std::cerr << "(identified via executable)\n";
            break;
        case IdentType::WINE_EXECUTABLE:
            std::cerr << "(identified via wine executable)\n";
            break;
        case IdentType::PROCESS_NAME:
            std::cerr << "(identified via process name)\n";
            break;
    }
}

bool Layer::tick() {
    auto res = this->config.tick();
    if (!res)
        return false;

    // try to find a profile
    const auto& detec = findProfile(this->config, identification);
    if (!detec.has_value())
        return this->profile.has_value();

    this->identType = detec->first;
    this->profile = detec->second;
    return true;
}

std::vector<const char*> Layer::instanceExtensions() const {
    if (!this->profile.has_value())
        return {};

    return {
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_external_semaphore_capabilities"
    };
}

std::vector<const char*> Layer::deviceExtensions() const {
    if (!this->profile.has_value())
        return {};

    return {
        "VK_KHR_external_memory",
        "VK_KHR_external_memory_fd",
        "VK_KHR_external_semaphore",
        "VK_KHR_external_semaphore_fd"
    };
}

layer::LayerInstance::LayerInstance(const Layer& layer, vk::VulkanDeviceFuncs df,
        VkInstance instance, VkDevice device,
        PFN_vkSetDeviceLoaderData setLoaderData) {
    std::cerr << "lsfg-vk: Hello, world!\n";
    std::cerr << "lsfg-vk: instance=" << instance << ", device=" << device << '\n';
    std::cerr << "lsfg-vk: setLoaderData=" << reinterpret_cast<void*>(setLoaderData) << '\n';
}
