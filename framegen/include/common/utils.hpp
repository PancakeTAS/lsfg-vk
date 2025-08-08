#pragma once

#include "core/commandbuffer.hpp"
#include "core/commandpool.hpp"
#include "core/descriptorpool.hpp"
#include "core/image.hpp"
#include "core/device.hpp"
#include "pool/resourcepool.hpp"
#include "pool/shaderpool.hpp"

#include <vulkan/vulkan_core.h>

#include <optional>
#include <cstddef>
#include <cstdint>
#include <string>
#include <array>
#include <vector>

namespace LSFG::Utils {

    ///
    /// Insert memory barriers for images in a command buffer.
    ///
    /// @throws std::logic_error if the command buffer is not in Recording state
    ///
    class BarrierBuilder {
    public:
        /// Create a barrier builder.
        BarrierBuilder(const Core::CommandBuffer& buffer)
                : commandBuffer(&buffer) {
            this->barriers.reserve(16); // this is performance critical
        }

        // Add a resource to the barrier builder.
        BarrierBuilder& addR2W(Core::Image& image);
        BarrierBuilder& addW2R(Core::Image& image);

        // Add an optional resource to the barrier builder.
        BarrierBuilder& addR2W(std::optional<Core::Image>& image) {
            if (image.has_value()) this->addR2W(*image); return *this; }
        BarrierBuilder& addW2R(std::optional<Core::Image>& image) {
            if (image.has_value()) this->addW2R(*image); return *this; }

        /// Add a list of resources to the barrier builder.
        BarrierBuilder& addR2W(std::vector<Core::Image>& images) {
            for (auto& image : images) this->addR2W(image); return *this; }
        BarrierBuilder& addW2R(std::vector<Core::Image>& images) {
            for (auto& image : images) this->addW2R(image); return *this; }

        /// Add an array of resources to the barrier builder.
        template<std::size_t N>
        BarrierBuilder& addR2W(std::array<Core::Image, N>& images) {
            for (auto& image : images) this->addR2W(image); return *this; }
        template<std::size_t N>
        BarrierBuilder& addW2R(std::array<Core::Image, N>& images) {
            for (auto& image : images) this->addW2R(image); return *this; }

        /// Finish building the barrier
        void build() const;
    private:
        const Core::CommandBuffer* commandBuffer;

        std::vector<VkImageMemoryBarrier2> barriers;
    };

    ///
    /// Upload a DDS file to a Vulkan image.
    ///
    /// @param device The Vulkan device
    /// @param commandPool The command pool
    /// @param image The Vulkan image to upload to
    /// @param path The path to the DDS file.
    ///
    /// @throws std::system_error If the file cannot be opened or read.
    /// @throws ls:vulkan_error If the Vulkan image cannot be created or updated.
    ///
    void uploadImage(const Core::Device& device,
        const Core::CommandPool& commandPool,
        Core::Image& image, const std::string& path);

    ///
    /// Clear a texture to white during setup.
    ///
    /// @param device The Vulkan device.
    /// @param image The image to clear.
    /// @param white If true, the image will be cleared to white, otherwise to black.
    ///
    /// @throws LSFG::vulkan_error If the Vulkan image cannot be cleared.
    ///
    void clearImage(const Core::Device& device, Core::Image& image, bool white = false);

}

namespace LSFG {
    struct Vulkan {
        Core::Device device;
        Core::CommandPool commandPool;
        Core::DescriptorPool descriptorPool;

        uint64_t generationCount;
        float flowScale;
        bool isHdr;

        Pool::ShaderPool shaders;
        Pool::ResourcePool resources;
    };
}
