/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "modules/pipeline/signature.hpp"

namespace lsfgvk {

    ///
    /// Get the pipeline signature
    ///
    /// @param perf Performance mode
    /// @return Pipeline signature
    ///
    const pipeline::PipelineSignature& getPipelineSignature(bool perf);

}
