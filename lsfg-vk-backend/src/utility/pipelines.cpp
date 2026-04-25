/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "pipelines.hpp"
#include "modules/pipeline/signature.hpp"
#include "modules/pipeline/signature/helpers.hpp"
#include "modules/pipeline/signature/image.hpp"
#include "modules/pipeline/signature/pass.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace lsfgvk;

namespace {
    using namespace lsfgvk::pipeline;

    /// Build the pipeline signature
    consteval PipelineSignature buildPipelineSignature(bool perf) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
        PipelineSignatureBuilder s;

        const Resource INVALID{};

        auto sourceImageArray = s.registerImage({
            .format = Format::RGBA8888,
            .hdrFormat = Format::RGBA16161616,
            .flags = ImageFlag::Pinned |
                ImageFlag::ExternalInput |
                ImageFlag::HdrVariant,
            .count = 2
        });

        /* Pre-pass */

        auto mipmapImageArray = s.registerImage({
            .format = Format::R8,
            .flags = ImageFlag::Mipmaps,
            .extentOp = { true },
            .count = 7
        });

        s.appendPass({
            .shader = "mipmaps",
            .inputs{
                sourceImageArray
            },
            .outputs{
                mipmapImageArray
            },
            .dispatchOp = { 63, 6 }
        });

        std::vector<size_t> alphaArray(7);
        std::vector<ExtentOp> alphaExtents(7);
        for (uint32_t i = 0; i < 7; i++) {
            const uint32_t mul = perf ? 1 : 2;
            const ExtentOp dispatch = { 7, 3 };

            ExtentOp extent = { 0, 6 - i };
            extent += { 1, 1 };

            auto flipflop0 = s.registerImage({
                .format = Format::RGBA8888,
                .extentOp = extent,
                .count = 1 * mul
            });

            s.appendPass({
                .shader = "alpha0",
                .flags = PassFlag::Aggregate,
                .inputs{
                    { mipmapImageArray, 6 - i }
                },
                .outputs{
                    flipflop0
                },
                .dispatchOp = extent + dispatch
            });

            auto flipflop1 = s.registerImage({
                .format = Format::RGBA8888,
                .extentOp = extent,
                .count = 1 * mul
            });

            s.appendPass({
                .shader = "alpha1",
                .flags = PassFlag::Aggregate,
                .inputs{
                    flipflop0
                },
                .outputs{
                    flipflop1
                },
                .dispatchOp = extent + dispatch
            });

            extent += { 1, 1 };

            auto flipflop2 = s.registerImage({
                .format = Format::RGBA8888,
                .extentOp = extent,
                .count = 2 * mul
            });

            s.appendPass({
                .shader = "alpha2",
                .flags = PassFlag::Aggregate,
                .inputs{
                    flipflop1
                },
                .outputs{
                    flipflop2
                },
                .dispatchOp = extent + dispatch
            });

            auto result = s.registerImage({
                .format = Format::RGBA8888,
                .flags = ImageFlag::Pinned,
                .extentOp = extent,
                .count = (2 * mul) * 3
            });

            s.appendPass({
                .shader = "alpha3",
                .flags = PassFlag::Aggregate,
                .inputs{
                    flipflop2
                },
                .outputs{
                    result
                },
                .dispatchOp = extent + dispatch
            });

            alphaArray.at(6 - i) = result;
            alphaExtents.at(6 - i) = extent;
        }

        ExtentOp extent = alphaExtents.at(0);
        ExtentOp dispatch = { 7, 3 };

        auto flipflop0 = s.registerImage({
            .format = Format::RGBA8888,
            .extentOp = extent,
            .count = 2
        });

        s.appendPass({
            .shader = "beta0",
            .inputs{
                alphaArray.at(0)
            },
            .outputs{
                flipflop0
            },
            .dispatchOp = extent + dispatch
        });

        auto flipflop1 = s.registerImage({
            .format = Format::RGBA8888,
            .extentOp = extent,
            .count = 2
        });

        s.appendPass({
            .shader = "beta1",
            .inputs{
                flipflop0
            },
            .outputs{
                flipflop1
            },
            .dispatchOp = extent + dispatch
        });

        auto flipflop2 = s.registerImage({
            .format = Format::RGBA8888,
            .extentOp = extent,
            .count = 2
        });

        s.appendPass({
            .shader = "beta2",
            .inputs{
                flipflop1
            },
            .outputs{
                flipflop2
            },
            .dispatchOp = extent + dispatch
        });

        auto flipflop3 = s.registerImage({
            .format = Format::RGBA8888,
            .extentOp = extent,
            .count = 2
        });

        s.appendPass({
            .shader = "beta3",
            .inputs{
                flipflop2
            },
            .outputs{
                flipflop3
            },
            .dispatchOp = extent + dispatch
        });

        auto betaImageArray = s.registerImage({
            .format = Format::R8,
            .flags = ImageFlag::Mipmaps,
            .extentOp = extent,
            .count = 6
        });

        dispatch = { 31, 5 };

        s.appendPass({
            .shader = "beta4",
            .inputs{
                flipflop3
            },
            .outputs{
                betaImageArray
            },
            .dispatchOp = extent + dispatch
        });

        /* Main-pass */

        s.split();

        std::vector<size_t> gammaArray(7);
        std::vector<size_t> deltaArray(3);
        std::vector<size_t> epsilonArray(3);
        for (uint32_t i = 0; i < 7; i++) {
            const uint32_t mul = perf ? 1 : 2;
            const ExtentOp dispatch = { 7, 3 };
            const ExtentOp extent = alphaExtents.at(6 - i);

            auto flipflop0 = s.registerImage({
                .format = Format::RGBA8888,
                .extentOp = extent,
                .count = 3
            });

            s.appendPass({
                .shader = "gamma0",
                .flags = PassFlag::Aggregate
                    | (i == 0 ? PassFlag::Special : PassFlag::None),
                .inputs{
                    alphaArray.at(6 - i),
                    i == 0 ? INVALID : gammaArray.at(i - 1)
                },
                .outputs{
                    flipflop0
                },
                .dispatchOp = extent + dispatch
            });

            auto flipflop1 = s.registerImage({
                .format = Format::RGBA8888,
                .extentOp = extent,
                .count = 2 * mul
            });

            s.appendPass({
                .shader = "gamma1",
                .flags = PassFlag::Aggregate,
                .inputs{
                    flipflop0
                },
                .outputs{
                    flipflop1
                },
                .dispatchOp = extent + dispatch
            });

            auto flipflop2 = s.registerImage({
                .format = Format::RGBA8888,
                .extentOp = extent,
                .count = 2 * mul
            });

            s.appendPass({
                .shader = "gamma2",
                .flags = PassFlag::Aggregate,
                .inputs{
                    flipflop1
                },
                .outputs{
                    flipflop2
                },
                .dispatchOp = extent + dispatch
            });

            auto flipflop3 = s.registerImage({
                .format = Format::RGBA8888,
                .extentOp = extent,
                .count = 2 * mul
            });

            s.appendPass({
                .shader = "gamma3",
                .flags = PassFlag::Aggregate,
                .inputs{
                    flipflop2
                },
                .outputs{
                    flipflop3
                },
                .dispatchOp = extent + dispatch
            });

            auto result = s.registerImage({
                .format = Format::RGBA16161616,
                .extentOp = extent
            });

            s.appendPass({
                .shader = "gamma4",
                .flags = PassFlag::Aggregate
                    | (i == 0 ? PassFlag::Special : PassFlag::None),
                .inputs{
                    flipflop3,
                    i == 0 ? INVALID : gammaArray.at(i - 1),
                    { betaImageArray, i == 0 ? 5 : (6 - i) }
                },
                .outputs{
                    result
                },
                .dispatchOp = extent + dispatch
            });

            gammaArray.at(i) = result;

            if (i >= 4) {
                auto flipflop0 = s.registerImage({
                    .format = Format::RGBA8888,
                    .extentOp = extent,
                    .count = 3
                });

                s.appendPass({
                    .shader = "delta0",
                    .flags = PassFlag::Aggregate
                        | (i == 4 ? PassFlag::Special : PassFlag::None),
                    .inputs{
                        alphaArray.at(6 - i),
                        i == 4 ? INVALID : deltaArray.at(i - 5)
                    },
                    .outputs{
                        flipflop0
                    },
                    .dispatchOp = extent + dispatch
                });

                auto flipflop1 = s.registerImage({
                    .format = Format::RGBA8888,
                    .extentOp = extent,
                    .count = 2 * mul
                });

                s.appendPass({
                    .shader = "delta1",
                    .flags = PassFlag::Aggregate,
                    .inputs{
                        flipflop0
                    },
                    .outputs{
                        flipflop1
                    },
                    .dispatchOp = extent + dispatch
                });

                auto flipflop2 = s.registerImage({
                    .format = Format::RGBA8888,
                    .extentOp = extent,
                    .count = 2 * mul
                });

                s.appendPass({
                    .shader = "delta2",
                    .flags = PassFlag::Aggregate,
                    .inputs{
                        flipflop1
                    },
                    .outputs{
                        flipflop2
                    },
                    .dispatchOp = extent + dispatch
                });

                auto flipflop3 = s.registerImage({
                    .format = Format::RGBA8888,
                    .extentOp = extent,
                    .count = 2 * mul
                });

                s.appendPass({
                    .shader = "delta3",
                    .flags = PassFlag::Aggregate,
                    .inputs{
                        flipflop2
                    },
                    .outputs{
                        flipflop3
                    },
                    .dispatchOp = extent + dispatch
                });

                auto result = s.registerImage({
                    .format = Format::RGBA16161616,
                    .extentOp = extent,
                    .count = 1
                });

                s.appendPass({
                    .shader = "delta4",
                    .flags = PassFlag::Aggregate
                        | (i == 4 ? PassFlag::Special : PassFlag::None),
                    .inputs{
                        flipflop3,
                        i == 4 ? INVALID : deltaArray.at(i - 5),
                        { betaImageArray, 6 - i }
                    },
                    .outputs{
                        result
                    },
                    .dispatchOp = extent + dispatch
                });

                deltaArray.at(i - 4) = result;
            }

            if (i >= 4) {
                auto flipflop0 = s.registerImage({
                    .format = Format::RGBA8888,
                    .extentOp = extent,
                    .count = mul
                });

                s.appendPass({
                    .shader = "epsilon0",
                    .flags = PassFlag::Aggregate
                        | (i == 4 ? PassFlag::Special : PassFlag::None),
                    .inputs{
                        alphaArray.at(6 - i),
                        gammaArray.at(i - 1),
                        i == 4 ? INVALID : deltaArray.at(i - 5)
                    },
                    .outputs{
                        flipflop0
                    },
                    .dispatchOp = extent + dispatch
                });

                auto flipflop1 = s.registerImage({
                    .format = Format::RGBA8888,
                    .extentOp = extent,
                    .count = mul
                });

                s.appendPass({
                    .shader = "epsilon1",
                    .flags = PassFlag::Aggregate,
                    .inputs{
                        flipflop0
                    },
                    .outputs{
                        flipflop1
                    },
                    .dispatchOp = extent + dispatch
                });

                auto flipflop2 = s.registerImage({
                    .format = Format::RGBA8888,
                    .extentOp = extent,
                    .count = mul
                });

                s.appendPass({
                    .shader = "epsilon2",
                    .flags = PassFlag::Aggregate,
                    .inputs{
                        flipflop1
                    },
                    .outputs{
                        flipflop2
                    },
                    .dispatchOp = extent + dispatch
                });

                auto flipflop3 = s.registerImage({
                    .format = Format::RGBA8888,
                    .extentOp = extent,
                    .count = mul
                });

                s.appendPass({
                    .shader = "epsilon3",
                    .flags = PassFlag::Aggregate,
                    .inputs{
                        flipflop2
                    },
                    .outputs{
                        flipflop3
                    },
                    .dispatchOp = extent + dispatch
                });

                auto result = s.registerImage({
                    .format = Format::RGBA16161616,
                    .extentOp = extent,
                    .count = 1
                });

                s.appendPass({
                    .shader = "epsilon4",
                    .flags = PassFlag::Aggregate
                        | (i == 4 ? PassFlag::Special : PassFlag::None),
                    .inputs{
                        flipflop3,
                        i == 4 ? INVALID : epsilonArray.at(i - 5)
                    },
                    .outputs{
                        result
                    },
                    .dispatchOp = extent + dispatch
                });

                epsilonArray.at(i - 4) = result;
            }
        }

        extent = { false };
        dispatch = { 15, 4 };

        auto result = s.registerImage({
            .format = Format::RGBA8888,
            .hdrFormat = Format::RGBA16161616,
            .flags = ImageFlag::Pinned
                | ImageFlag::ExternalOutput
                | ImageFlag::HdrVariant,
            .extentOp = extent,
            .count = 2 // FIXME: Count should be 1.
        });

        s.appendPass({
            .shader = "generate",
            .flags = PassFlag::HdrVariant,
            .inputs{
                sourceImageArray,
                gammaArray.at(6),
                deltaArray.at(2),
                epsilonArray.at(2)
            },
            .outputs{
                result
            },
            .dispatchOp = extent + dispatch
        });

        return s.finalize();
#pragma clang diagnostic pop
    }
}

const PipelineSignature& lsfgvk::getPipelineSignature(bool perf) {
    static const PipelineSignature signature = buildPipelineSignature(false);
    static const PipelineSignature perfSignature = buildPipelineSignature(true);
    return perf ? perfSignature : signature;
}
