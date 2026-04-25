/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace lsfgvk::pipeline {

    /// All supported image formats
    enum class Format : char {
        /// Invalid format
        Invalid = 0,
        /// 8-bit unsigned normalized RGBA format
        RGBA8888 = 37, // VK_FORMAT_R8G8B8A8_UNORM
        /// 8-bit unsigned normalized R format
        R8 = 9, // VK_FORMAT_R8_UNORM
        /// 16-bit signed floating point RGBA format
        RGBA16161616 = 97, // VK_FORMAT_R16G16B16A16_SFLOAT
    };

    /// All supported image flags
    enum class ImageFlag : char {
        /// No special flags
        None = 0,
        /// Instead of using a single image array, create several individual images with halving
        /// extends for each mip level.
        ///
        /// This will cause the image to show up as Texture2D[], rather than Texture2DArray
        /// and must therefore not be used in full with passes where the "Aggregate" flag is set.
        Mipmaps = 1 << 0,
        /// Indicate that the image is pinned & not transient
        Pinned = 1 << 1,
        /// Indicate that this image is written to externally
        ExternalInput = 1 << 2,
        /// Indicate that this image is read from externally
        ExternalOutput = 1 << 3,
        /// Indicate that a separate format should be used for HDR
        HdrVariant = 1 << 4
    };

    /// Helper type for operating on image flags
    class ImageFlags {
    public:
        /// Default constructor
        constexpr ImageFlags() = default;
        /// Create from single image flag
        constexpr ImageFlags(ImageFlag flag) : m_flags(static_cast<int>(flag)) {}
        /// Check any set of flags
        constexpr operator bool() const { return m_flags != 0; }
        /// Combine with another flag
        constexpr ImageFlags operator|(ImageFlag flag) const {
            return{this->m_flags | static_cast<int>(flag)};
        }
        /// Match with another flag
        constexpr ImageFlags operator&(ImageFlag flag) const {
            return{this->m_flags & static_cast<int>(flag)};
        }
        /// Match with another flag instance
        constexpr ImageFlags operator&(ImageFlags other) const {
            return{this->m_flags & other.m_flags};
        }
    private:
        int m_flags{static_cast<int>(ImageFlag::None)};

        // Create from number
        constexpr ImageFlags(int flags) : m_flags(flags) {}
    };

    /// Compine two image flags
    constexpr ImageFlags operator|(ImageFlag lhs, ImageFlag rhs) {
        return ImageFlags(lhs) | rhs;
    }

    /// Signature for an image
    struct ImageSignature {
        /// Format of the image
        Format format{ Format::RGBA8888 };
        /// Optional second format for HDR variants
        Format hdrFormat{ Format::RGBA16161616 };
        /// Optional flags for the image
        ImageFlags flags{ ImageFlag::None };
        /// Operation applied to the base extent for calculating the image extent
        ExtentOp extentOp;
        /// Amount of layers in the image
        uint32_t count{1};

        /// Lifetime of the image (set by pipeline builder)
        std::pair<size_t, size_t> lifetime;
    };

}
