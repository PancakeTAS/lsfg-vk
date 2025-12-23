#pragma once

#include <QStringList>

namespace lsfgvk::ui {

    /// get the list of available GPUs, automatically
    /// switching to PCI IDs if there are duplicates
    /// @return list of available GPUs
    QStringList getAvailableGPUs();

}
