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

            // Calculate pipeline stages by reordering passes with dependencies as constraints
            std::vector<size_t> writtenImages;
            for (size_t i = 0; i < this->m_images.size(); i++) {
                const auto& image{this->m_images.at(i)};
                if (image.flags & ImageFlag::ExternalInput)
                    writtenImages.push_back(i);
            }

            std::vector<size_t> remainingPasses(this->m_passes.size());
            std::iota(remainingPasses.begin(), remainingPasses.end(), 0);

            size_t currentStageIndex{0};
            std::pair<size_t, size_t> currentStageBounds{
                0,
                this->m_splitIndices.empty() ? this->m_passes.size() : this->m_splitIndices.front()
            };

            while (!remainingPasses.empty()) {
                auto& currentStage{s.stages.emplace_back()};

                // Find all passes that may be executed next
                std::vector<size_t> validPasses{};
                for (const auto& passIdx : remainingPasses) {
                    if (passIdx < currentStageBounds.first || passIdx >= currentStageBounds.second)
                        continue; // Skip passes that are not in the current stage

                    const auto& pass{this->m_passes.at(passIdx)};

                    bool isValid{true};
                    for (const auto& image : pass.inputs) {
                        if (!image.idx())
                            continue;
                        if (std::ranges::find(writtenImages, *image.idx()) != writtenImages.end())
                            continue;

                        isValid = false;
                        break;
                    }

                    if (!isValid)
                        continue;

                    validPasses.push_back(passIdx);
                }

                // If no valid pass exists in the current stage, move on to the next stage
                if (validPasses.empty() && currentStageIndex < this->m_splitIndices.size()) {
                    currentStageIndex++;
                    currentStageBounds = {
                        currentStageBounds.second,
                        currentStageIndex < this->m_splitIndices.size() ?
                            this->m_splitIndices.at(currentStageIndex) : this->m_passes.size()
                    };

                    s.stages.pop_back();
                    s.splitIndices.emplace_back(s.stages.size());
                    continue;
                }

                // Sort valid passes by shader name
                auto begin = std::ranges::begin(validPasses);
                auto end = std::ranges::end(validPasses);
                for (auto i = begin; i != end; i++) {
                    std::rotate(
                        std::upper_bound(begin, i, *i, [this](size_t a, size_t b) {
                            return this->m_passes.at(a).shader < this->m_passes.at(b).shader;
                        }),
                        i, std::next(i)
                    );
                }

                // Merge passes into execution step
                for (const auto& passIdx : validPasses) {
                    const auto& pass{this->m_passes.at(passIdx)};

                    for (const auto& resource : pass.outputs) {
                        if (!resource.idx())
                            continue;
                        writtenImages.push_back(*resource.idx());
                    }

                    currentStage.passes.push_back(passIdx);
                    remainingPasses.erase(std::ranges::find(remainingPasses, passIdx));
                }
            }

            // Calculate usage timeline for each image
            for (size_t i = 0; i < this->m_images.size(); i++) {
                auto& image{this->m_images.at(i)};
                if (image.flags & ImageFlag::Pinned)
                    continue;

                std::optional<size_t> writeIndex;
                std::optional<size_t> readIndex;

                // Find the first stage that writes to the image and last stage that reads from it
                for (size_t j = 0; j < s.stages.size(); j++) {
                    const auto& stage{s.stages.at(j)};

                    for (const auto& passIdx : stage.passes) {
                        const auto& pass{this->m_passes.at(passIdx)};

                        const bool isRead{
                            std::ranges::any_of(pass.inputs, [i](const auto& resource) {
                                return resource.idx() && *resource.idx() == i;
                            })
                        };
                        const bool isWritten{
                            std::ranges::any_of(pass.outputs, [i](const auto& resource) {
                                return resource.idx() && *resource.idx() == i;
                            })
                        };

                        if (writeIndex && isWritten)
                            throw "Image " + std::to_string(i) +
                                " is written by multiple passes";
                        if (isWritten && isRead)
                            throw "Image " + std::to_string(i) +
                                " is read & write in the same pass";

                        if (isWritten)
                            writeIndex.emplace(j);
                        if (isRead)
                            readIndex.emplace(std::max(readIndex.value_or(0), j));
                    }
                }

                if (!writeIndex)
                    throw "Image " + std::to_string(i) + " is not written to by any pass";
                if (!readIndex)
                    throw "Image " + std::to_string(i) + " is not read from by any pass";

                image.lifetime = { *writeIndex, *readIndex };
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
