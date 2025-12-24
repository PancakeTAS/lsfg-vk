/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "tools/benchmark.hpp"
#include "tools/debug.hpp"
#include "tools/validate.hpp"

#include <array>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#include <getopt.h> // NOLINT
#include <bits/getopt_core.h>
#include <bits/getopt_ext.h>

using namespace lsfgvk::cli;

namespace {
    /// print usage information
    void usage(const std::string& prog) {
        std::cerr <<
R"(Validate, benchmark, and debug lsfg-vk.

USAGE:
    )" << prog << R"( <COMMAND> [OPTIONS] [ARGS]

COMMANDS:
    validate    Validate a configuration file
    benchmark   Run a benchmark
    debug       Run lsfg-vk on a set of images

SUBCOMMAND OPTIONS:

    validate
        -c, --config <PATH>             Optional path to the configuration file

    benchmark & debug
        -d, --dll <PATH>                Path to Lossless.dll
        -a, --allow-fp16                Allow FP16 acceleration
        -w, --width <INT>               Width of the input frames
        -h, --height <INT>              Height of the input frames
        -f, --flow <FLOAT>              Flow scale
        -m, --multiplier <INT>          Multiplier
        -p, --performance-mode          Use performance mode
        -g, --gpu <STRING>              GPU to use

    benchmark
        -t, --duration <SECONDS>        Benchmark duration in seconds

    debug
        <folder>                        Path to the debug frames)" << '\n';
    }

    /// parse the validate command options
    [[noreturn]] void on_validate(int argc, char** argv) {
        validate::Options opts{};

        const std::array<option, 3> GETOPT {{
            { "config", required_argument, nullptr, 'c' },
            { nullptr,        no_argument, nullptr,  0  }
        }};

        int c{0};
        while ((c = getopt_long(argc, argv, "c:", GETOPT.data(), nullptr)) != -1) {
            switch (c) {
                case 'c':
                    opts.config.emplace(optarg);
                    break;
                case '?':
                default:
                    usage(*argv);
                    std::exit(EXIT_FAILURE);
            }
        }

        if (optind < argc) {
            usage(*argv);
            std::exit(EXIT_FAILURE);
        }

        std::exit(validate::run(opts));
    }

    /// parse the benchmark command options
    [[noreturn]] void on_benchmark(int argc, char** argv) {
        benchmark::Options opts{};

        const std::array<option, 10> GETOPT {{
            { "dll",              required_argument, nullptr, 'd' },
            { "allow-fp16",       no_argument,       nullptr, 'a' },
            { "width",            required_argument, nullptr, 'w' },
            { "height",           required_argument, nullptr, 'h' },
            { "flow",             required_argument, nullptr, 'f' },
            { "multiplier",       required_argument, nullptr, 'm' },
            { "performance-mode",       no_argument, nullptr, 'p' },
            { "gpu",              required_argument, nullptr, 'g' },
            { "duration",         required_argument, nullptr, 't' },
            { nullptr,                  no_argument, nullptr,  0  }
        }};

        int c{0};
        while ((c = getopt_long(argc, argv, "d:aw:h:f:m:pg:t:", GETOPT.data(), nullptr)) != -1) {
            switch (c) {
                case 'd':
                    opts.dll.emplace(optarg);
                    break;
                case 'a':
                    opts.allow_fp16 = true;
                    break;
                case 'w':
                    opts.width = std::stoi(optarg);
                    break;
                case 'h':
                    opts.height = std::stoi(optarg);
                    break;
                case 'f':
                    opts.flow = std::stof(optarg);
                    break;
                case 'm':
                    opts.multiplier = std::stoi(optarg);
                    break;
                case 'p':
                    opts.performance_mode = true;
                    break;
                case 'g':
                    opts.gpu.emplace(optarg);
                    break;
                case 't':
                    opts.duration = std::stoi(optarg);
                    break;
                case '?':
                default:
                    usage(*argv);
                    std::exit(EXIT_FAILURE);
            }
        }

        if (optind < argc) {
            usage(*argv);
            std::exit(EXIT_FAILURE);
        }

        std::exit(benchmark::run(opts));
    }

    /// parse the debug command options
    [[noreturn]] void on_debug(int argc, char** argv) {
        debug::Options opts{};

        const std::array<option, 9> GETOPT {{
            { "dll",              required_argument, nullptr, 'd' },
            { "allow-fp16",       no_argument,       nullptr, 'a' },
            { "width",            required_argument, nullptr, 'w' },
            { "height",           required_argument, nullptr, 'h' },
            { "flow",             required_argument, nullptr, 'f' },
            { "multiplier",       required_argument, nullptr, 'm' },
            { "performance-mode",       no_argument, nullptr, 'p' },
            { "gpu",              required_argument, nullptr, 'g' },
            { nullptr,                  no_argument, nullptr,  0  }
        }};

        int c{0};
        while ((c = getopt_long(argc, argv, "d:aw:h:f:m:pg:", GETOPT.data(), nullptr)) != -1) {
            switch (c) {
                case 'd':
                    opts.dll.emplace(optarg);
                    break;
                case 'a':
                    opts.allow_fp16 = true;
                    break;
                case 'w':
                    opts.width = std::stoi(optarg);
                    break;
                case 'h':
                    opts.height = std::stoi(optarg);
                    break;
                case 'f':
                    opts.flow = std::stof(optarg);
                    break;
                case 'm':
                    opts.multiplier = std::stoi(optarg);
                    break;
                case 'p':
                    opts.performance_mode = true;
                    break;
                case 'g':
                    opts.gpu.emplace(optarg);
                    break;
                case '?':
                default:
                    usage(*argv);
                    std::exit(EXIT_FAILURE);
            }
        }

        if ((optind + 1) != argc) {
            usage(*argv);
            std::exit(EXIT_FAILURE);
        }

        opts.path = argv[optind];

        std::exit(debug::run(opts));
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage(*argv);
        return EXIT_FAILURE;
    }

    const std::string command{argv[1]};
    if (command == "validate")
        on_validate(argc - 1, argv + 1);
    else if (command == "benchmark")
        on_benchmark(argc - 1, argv + 1);
    else if (command == "debug")
        on_debug(argc - 1, argv + 1);

    usage(*argv);
    return EXIT_FAILURE;
}
