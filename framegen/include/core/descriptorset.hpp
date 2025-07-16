#pragma once

#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/descriptorpool.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/sampler.hpp"
#include "core/shadermodule.hpp"
#include "core/device.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>
#include <cstddef>
#include <array>
#include <optional>
#include <memory>

namespace LSFG::Core {

    class DescriptorSetUpdateBuilder;

    ///
    /// C++ wrapper class for a Vulkan descriptor set.
    ///
    /// This class manages the lifetime of a Vulkan descriptor set.
    ///
    class DescriptorSet {
    public:
        DescriptorSet() noexcept = default;

        ///
        /// Create the descriptor set.
        ///
        /// @param device Vulkan device
        /// @param pool Descriptor pool to allocate from
        /// @param shaderModule Shader module to use for the descriptor set
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        DescriptorSet(const Core::Device& device,
            const DescriptorPool& pool, const ShaderModule& shaderModule);

        ///
        /// Update the descriptor set with resources.
        ///
        /// @param device Vulkan device
        ///
        [[nodiscard]] DescriptorSetUpdateBuilder update(const Core::Device& device) const;

        ///
        /// Bind a descriptor set to a command buffer.
        ///
        /// @param commandBuffer Command buffer to bind the descriptor set to.
        /// @param pipeline Pipeline to bind the descriptor set to.
        ///
        void bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline) const;

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->descriptorSet; }

        /// Trivially copyable, moveable and destructible
        DescriptorSet(const DescriptorSet&) noexcept = default;
        DescriptorSet& operator=(const DescriptorSet&) noexcept = default;
        DescriptorSet(DescriptorSet&&) noexcept = default;
        DescriptorSet& operator=(DescriptorSet&&) noexcept = default;
        ~DescriptorSet() = default;
    private:
        std::shared_ptr<VkDescriptorSet> descriptorSet;
    };

    ///
    /// Builder class for updating a descriptor set.
    ///
    class DescriptorSetUpdateBuilder {
        friend class DescriptorSet;
    public:
        /// Add a resource to the descriptor set update.
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const Image& image);
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const Sampler& sampler);
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const Buffer& buffer);
        DescriptorSetUpdateBuilder& add(VkDescriptorType type); // empty entry

        /// Add a list of resources to the descriptor set update.
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::vector<Image>& images) {
            for (const auto& image : images) this->add(type, image); return *this; }
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::vector<Sampler>& samplers) {
            for (const auto& sampler : samplers) this->add(type, sampler); return *this; }
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::vector<Buffer>& buffers) {
            for (const auto& buffer : buffers) this->add(type, buffer); return *this; }

        /// Add an array of resources
        template<std::size_t N>
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::array<Image, N>& images) {
            for (const auto& image : images) this->add(type, image); return *this; }
        template<std::size_t N>
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::array<Sampler, N>& samplers) {
            for (const auto& sampler : samplers) this->add(type, sampler); return *this; }
        template<std::size_t N>
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::array<Buffer, N>& buffers) {
            for (const auto& buffer : buffers) this->add(type, buffer); return *this; }

        /// Add an optional resource to the descriptor set update.
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::optional<Image>& image) {
            if (image.has_value()) this->add(type, *image); else this->add(type); return *this; }
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::optional<Sampler>& sampler) {
            if (sampler.has_value()) this->add(type, *sampler); else this->add(type); return *this; }
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::optional<Buffer>& buffer) {
            if (buffer.has_value()) this->add(type, *buffer); else this->add(type); return *this; }

        /// Finish building the descriptor set update.
        void build();
    private:
        const DescriptorSet* descriptorSet;
        const Core::Device* device;

        DescriptorSetUpdateBuilder(const DescriptorSet& descriptorSet, const Core::Device& device)
                : descriptorSet(&descriptorSet), device(&device) {}

        std::vector<VkWriteDescriptorSet> entries;
    };

}
