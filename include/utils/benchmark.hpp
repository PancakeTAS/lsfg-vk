#pragma once

#include <cstdint>

namespace Benchmark {

    ///
    /// Run the benchmark.
    ///
    /// @param width The width of the benchmark.
    /// @param height The height of the benchmark.
    ///
    [[noreturn]] void run(uint32_t width, uint32_t height);

}
