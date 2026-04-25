/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <QtContainerFwd>
#include <QStringList>
#include <QString>

#include "utils.hpp"

#include <cstddef>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_DEFAULT_DISPATCHER 1
#define VULKAN_HPP_NO_CONSTRUCTORS 1
#include <vulkan/vulkan.hpp>

using namespace lsfgvk;
using namespace lsfgvk::ui;

QStringList ui::queryGPUs() {
    // Create a Vulkan instance
    vk::detail::DispatchLoaderDynamic dld;
    dld.init();

    const vk::ApplicationInfo appInfo{
        .pApplicationName = "lsfg-vk-ui",
        .applicationVersion = vk::makeVersion(2, 0, 0),
        .pEngineName = "lsfg-vk-ui",
        .engineVersion = vk::makeVersion(2, 0, 0),
        .apiVersion = vk::ApiVersion12 // Required by lsfg-vk anyways
    };
    const vk::InstanceCreateInfo instanceInfo{
        .pApplicationInfo = &appInfo
    };
    const vk::UniqueInstance instance{vk::createInstanceUnique(instanceInfo, nullptr, dld)};
    dld.init(*instance);

    // Query physical devices
    std::vector<std::string> devicesByName{};
    std::vector<std::string> devicesByBusId{};

    for (const auto& physdev : instance->enumeratePhysicalDevices(dld)) {
        // Check for VK_EXT_pci_bus_info
        bool supportsPCIEXT{false};
        for (const auto& ext : physdev.enumerateDeviceExtensionProperties(nullptr, dld)) {
            if (std::string(ext.extensionName) != vk::EXTPciBusInfoExtensionName)
                continue;

            supportsPCIEXT = true;
            break;
        }

        // Fetch properties
        vk::PhysicalDevicePCIBusInfoPropertiesEXT busInfo{};
        vk::PhysicalDeviceProperties2 info{
            .pNext = supportsPCIEXT ? &busInfo : nullptr
        };
        physdev.getProperties2(&info, dld);

        auto& props{info.properties};

        // Append device name
        props.deviceName.back() = '\0'; // Ensure null-termination
        devicesByName.emplace_back(props.deviceName);

        // Append PCI bus ID
        if (!supportsPCIEXT)
            continue;

        std::ostringstream pciss;
        pciss << std::hex << std::setfill('0')
            << std::setw(4) << busInfo.pciDomain << ":"
            << std::setw(2) << busInfo.pciBus << ":"
            << std::setw(2) << busInfo.pciDevice << "."
            << std::setw(1) << busInfo.pciFunction;
        devicesByBusId.emplace_back(pciss.str());
    }

    // Count duplicate names
    std::unordered_map<std::string, size_t> repeats{};
    for (const auto& name : devicesByName)
        repeats[name]++;

    // Build the frontend list
    QStringList list{"Default"};
    for (size_t i = 0; i < devicesByName.size(); i++) {
        const auto& name{devicesByName.at(i)};

        // Decide whether to show PCI bus ID or device name
        QString entry;
        if (repeats[name] > 1)
            entry = QString::fromStdString(devicesByBusId.at(i));
        else
            entry = QString::fromStdString(name);

        // Append to list if not already present (flatpak does funny things)
        if (list.contains(entry))
            continue;
        list.append(entry);
    }

    return list;
}
