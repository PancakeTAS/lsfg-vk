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
        if (count > 1 && gpu.second.has_value())
            list.append(QString::fromStdString(*gpu.second));
        else
            list.append(QString::fromStdString(gpu.first));
    }

    return list;
}
