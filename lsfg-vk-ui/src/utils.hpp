/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <QStringList>

namespace lsfgvk::ui {

    ///
    /// Query all GPUs available on the system.
    ///
    /// @return List of available GPUs
    ///
    QStringList queryGPUs();

}
