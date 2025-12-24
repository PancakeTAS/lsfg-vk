#include <QStringList>
#include <QString>

#include "utils.hpp"
#include "lsfg-vk-backend/lsfgvk.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace lsfgvk;
using namespace lsfgvk::ui;

QStringList ui::getAvailableGPUs() { // NOLINT (IWYU)
    // list of found GPUs and their optional PCI IDs
    std::vector<std::pair<std::string, std::optional<std::string>>> gpus{};

    // create a backend to query all GPUs
    try {
        const backend::DevicePicker picker{[&gpus](
            const std::string& deviceName,
            std::pair<const std::string&, const std::string&>,
            const std::optional<std::string>& pci
        ) {
            gpus.emplace_back(deviceName, pci);
            return false; // always fail
        }};

        const backend::Instance instance{picker, "/non/existent/path", false};
        throw std::runtime_error("???");
    } catch (const backend::error&) { // NOLINT
        // expected
    }

    // first remove 1:1 duplicates
    std::ranges::sort(gpus);
    gpus.erase(std::ranges::unique(gpus).begin(), gpus.end());

    // build the frontend list
    QStringList list{"Default"}; // NOLINT (IWYU)
    for (const auto& gpu : gpus) {
        // check if GPU is in list more than once
        auto count = std::count_if(gpus.begin(), gpus.end(),
            [&gpu](const auto& other) {
                return other.first == gpu.first;
            }
        );

        // add pci id to distinguish, otherwise add just the name
        QString entry;
        if (count > 1 && gpu.second.has_value())
            entry = QString::fromStdString(*gpu.second);
        else
            entry = QString::fromStdString(gpu.first);

        // ensure no duplicates (flatpak does funny things)
        if (list.contains(entry))
            continue;
        list.append(entry);
    }

    return list;
}
