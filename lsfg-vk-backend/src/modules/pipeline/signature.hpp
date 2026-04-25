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

            struct ShaderInfo {
                std::string_view id;
                bool hasHdrVariant{};
                size_t sampledImageBindings{}; // Only the amount suffices here
                std::vector<std::vector<size_t>> storageImageBindings;
            };
            std::vector<ShaderInfo> shaderInfos;

            // Populate shader map with empty bindings
            for (const auto& pass : this->m_passes) {
                const auto it{std::ranges::find_if(shaderInfos, [&pass](const auto& shader) {
                    return shader.id == pass.shader;
                })};
                const bool firstOccurrence{it == shaderInfos.end()};
                const bool isAggregatePass{pass.flags & PassFlag::Aggregate};

                auto& shader{firstOccurrence ? shaderInfos.emplace_back() : *it};

                if (firstOccurrence) {
                    shader.id = pass.shader;
                    shader.hasHdrVariant = pass.flags & PassFlag::HdrVariant;
                    shader.sampledImageBindings = pass.inputs.size();
                    shader.storageImageBindings.resize(pass.outputs.size());
                }

                // Ensure consistent usage aross invocations
                if (!firstOccurrence && !isAggregatePass)
                    throw "Shader \"" + std::string(pass.shader) + "\" is used by "
                        "multiple passes but does not have the Aggregate flag set";

                if (shader.sampledImageBindings != pass.inputs.size())
                    throw "Shader \"" + std::string(pass.shader) + "\" has "
                        "inconsistent read counts across passes";
                if (shader.storageImageBindings.size() != pass.outputs.size())
                    throw "Shader \"" + std::string(pass.shader) + "\" has "
                        "inconsistent write counts across passes";

                // Collect all used resources written by this shader
                for (size_t i = 0; i < pass.outputs.size(); i++) {
                    const auto& resource{pass.outputs.at(i)};
                    if (!resource.idx())
                        continue;

                    const auto& image{this->m_images.at(*resource.idx())};
                    if (isAggregatePass && (image.flags & ImageFlag::Mipmaps) && !resource.layer())
                        throw "Pass \"" + std::string(pass.shader) + "\" has "
                            "Aggregate flag but fully writes to an image with Mipmaps flag";

                    shader.storageImageBindings.at(i).push_back(*resource.idx());
                }
            }

            // Create descriptors for all resources
            for (size_t i = 0; i < this->m_images.size(); i++) {
                const auto& image{this->m_images.at(i)};
                if (image.flags & ImageFlag::ExternalInput) {
                    s.descriptors.push_back({
                        .type = BindingType::SampledImage,
                        .resources = { i }
                    });
                }
            }
            for (const auto& shader : shaderInfos) {
                for (const auto& resources : shader.storageImageBindings) {
                    s.descriptors.push_back({
                        .type = BindingType::StorageImage,
                        .resources = resources
                    });

                    // Skip sampled image bindings for external outputs
                    const auto& image{this->m_images.at(resources.front())};
                    if (image.flags & ImageFlag::ExternalOutput)
                        continue;

                    s.descriptors.push_back({
                        .type = BindingType::SampledImage,
                        .resources = resources
                    });
                }
            }

            // Copy remaining resources into signature
            for (const auto& shader : shaderInfos)
                s.shaders.emplace_back(shader.id, shader.hasHdrVariant);
            for (const auto& image : this->m_images)
                s.images.push_back(image);
            for (const auto& pass : this->m_passes)
                s.passes.push_back(pass);
            return s;
        }
    private:
        std::vector<ImageSignature> m_images;
        std::vector<PassSignature> m_passes;
        std::vector<size_t> m_splitIndices;
    };

}
