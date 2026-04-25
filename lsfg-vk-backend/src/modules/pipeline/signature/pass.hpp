/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace lsfgvk::pipeline {

    /// All supported pass flags
    enum class PassFlag : char {
        /// No special flags
        None = 0,
        /// Indicates the shader will be reused several times and resources must be
        /// aggregated into arrays and indexed via push constants.
        Aggregate = 1 << 0,
        /// Indicate that the special flag is set via push constant.
        Special = 1 << 1,
        /// Indicate that there are two variants for 8-bit and 16-bit foramtrs
        HdrVariant = 1 << 2
    };

    /// Helper type for operating on pass flags
    class PassFlags {
    public:
        /// Default constructor
        constexpr PassFlags() = default;
        /// Create from single pass flag
        constexpr PassFlags(PassFlag flag) : m_flags(static_cast<int>(flag)) {}
        /// Check any set of flags
        constexpr operator bool() const { return m_flags != 0; }
        /// Combine with another flag
        constexpr PassFlags operator|(PassFlag flag) const {
            return{this->m_flags | static_cast<int>(flag)};
        }
        /// Match with another flag
        constexpr PassFlags operator&(PassFlag flag) const {
            return{this->m_flags & static_cast<int>(flag)};
        }
    private:
        int m_flags{static_cast<int>(PassFlag::None)};

        // Create from number
        constexpr PassFlags(int flags) : m_flags(flags) {}
    };

    /// Combine two pass flags
    constexpr PassFlags operator|(PassFlag lhs, PassFlag rhs) {
        return PassFlags(lhs) | rhs;
    }

    /// A pointer to an image, or a specific layer inside that image
    class Resource {
    public:
        /// Default constructor
        constexpr Resource() = default;
        /// Constructor for a full image
        constexpr Resource(size_t idx) : m_idx(idx) {}
        /// Constructor for a single layer
        constexpr Resource(size_t idx, uint32_t layer) : m_idx(idx), m_layer(layer) {}
        /// Get the flow value
        [[nodiscard]] constexpr auto idx() const { return this->m_idx; }
        /// Get the operations
        [[nodiscard]] constexpr auto layer() const { return this->m_layer; }
    private:
        std::optional<size_t> m_idx{0};
        std::optional<uint32_t> m_layer;
    };

    /// Signature of a shader pass
    struct PassSignature {
        /// Name of the shader
        std::string_view shader;
        /// Optional flags of this pass
        PassFlags flags{ PassFlag::None };
        /// Resources to read from
        inplace_vector<Resource, 8> inputs;
        /// Resources to write to
        inplace_vector<Resource, 8> outputs;
        /// Operation applied to the base extent for calculating the dispatch extent
        ExtentOp dispatchOp;
    };

}
