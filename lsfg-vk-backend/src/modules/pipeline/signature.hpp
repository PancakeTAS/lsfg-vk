/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "signature/helpers.hpp"
#include "signature/image.hpp"
#include "signature/pass.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lsfgvk::pipeline {

    /// Type of a descriptor set binding
    enum class BindingType : uint8_t {
        SampledImage,
        StorageImage
    };

    /// Signature of a descriptor set binding
    struct BindingSignature {
        /// Type of binding
        BindingType type{ BindingType::SampledImage };
        /// Resources attached to binding
        inplace_vector<size_t, 16> resources;
    };

    /// Signature of a pipeline stage
    struct StageSignature {
        /// Passes executed this stage
        inplace_vector<size_t, 8> passes;
    };

    ///
    /// Signature of a compute pipeline
    ///
    struct PipelineSignature {
        /// Shader names used by the pipeline (and if there are hdr variants)
        inplace_vector<std::pair<std::string_view, bool>, 32> shaders;
        /// Images used by the pipeline
        inplace_vector<ImageSignature, 192> images;
        /// Ordered set of bindings for the descriptor set
        inplace_vector<BindingSignature, 192> descriptors;
        /// Indexable list of all passes
        inplace_vector<PassSignature, 100> passes;
        /// Ordered list of stages, excecuted in sequence
        inplace_vector<StageSignature, 100> stages;
        /// Stage index where the command buffers are split
        inplace_vector<size_t, 4> splitIndices;
    };

    ///
    /// The signature of a compute pipeline
    ///
    class PipelineSignatureBuilder {
    public:
        ///
        /// Create a new empty signature builder
        ///
        explicit PipelineSignatureBuilder() = default;

        ///
        /// Register an image
        ///
        /// @param image Image signature
        /// @return Handle to the image
        ///
        consteval size_t registerImage(ImageSignature image) {
            this->m_images.push_back(std::move(image));
            return this->m_images.size() - 1;
        }

        ///
        /// Append a pass
        ///
        /// @param pass Pass signature
        /// @return Handle to the pass
        ///
        consteval size_t appendPass(PassSignature pass) {
            this->m_passes.push_back(std::move(pass));
            return this->m_passes.size() - 1;
        }

        ///
        /// Split the command buffer
        ///
        consteval void split() {
            this->m_splitIndices.emplace_back(this->m_passes.size());
        }

        ///
        /// Compute a pipeline signature
        ///
        /// @throws const char* on failure
        /// @return Pipeline siganture
        ///
        consteval PipelineSignature finalize() {
            PipelineSignature s{};

            /* TODO */

            return s;
        }
    private:
        std::vector<ImageSignature> m_images;
        std::vector<PassSignature> m_passes;
        std::vector<size_t> m_splitIndices;
    };

}
