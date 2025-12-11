#include "layer.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

using namespace lsfgvk;

std::vector<std::string> lsfgvk::requiredInstanceExtensions() noexcept {
    return {};
}

std::vector<std::string> lsfgvk::requiredDeviceExtensions() noexcept {
    return {};
}

lsfgvk::LayerInstance::LayerInstance(VkInstance instance, VkDevice device,
        PFN_vkSetDeviceLoaderData setLoaderData) {
    std::cerr << "lsfg-vk: Hello, world!\n";
    std::cerr << "lsfg-vk: instance=" << instance << ", device=" << device << '\n';
    std::cerr << "lsfg-vk: setLoaderData=" << reinterpret_cast<void*>(setLoaderData) << '\n';
}
