#include "lsfg-vk-backend/lsfgvk.hpp"
#include "extraction/dll_reader.hpp"
#include "extraction/shader_registry.hpp"
#include "helpers/utils.hpp"
#include "lsfg-vk-common/helpers/errors.hpp"
#include "lsfg-vk-common/vulkan/command_buffer.hpp"
#include "lsfg-vk-common/vulkan/image.hpp"
#include "lsfg-vk-common/vulkan/timeline_semaphore.hpp"
#include "lsfg-vk-common/vulkan/vulkan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

#ifdef LSFGVK__RENDERDOC_INTEGRATION
#include <renderdoc_app.h>
#include <dlfcn.h>
#endif

using namespace lsfgvk;

namespace lsfgvk {
    /// instance class
    class InstanceImpl {
    public:
        /// create an instance
        /// (see lsfg-vk documentation)
        InstanceImpl(vk::PhysicalDeviceSelector selectPhysicalDevice,
            const std::filesystem::path& shaderDllPath,
            bool allowLowPrecision);

        /// get the Vulkan instance
        /// @return the Vulkan instance
        [[nodiscard]] const auto& getVulkan() const { return this->vk; }
        /// get the shader registry
        /// @return the shader registry
        [[nodiscard]] const auto& getShaderRegistry() const { return this->shaders; }
#ifdef LSFGVK__RENDERDOC_INTEGRATION
        /// get the RenderDoc API
        /// @return the RenderDoc API
        [[nodiscard]] const auto& getRenderDocAPI() const { return this->renderdoc; }
#endif
    private:
        vk::Vulkan vk;
        extr::ShaderRegistry shaders;

#ifdef LSFGVK__RENDERDOC_INTEGRATION
        std::optional<RENDERDOC_API_1_6_0> renderdoc;
#endif
    };

    /// context class
    class ContextImpl {
    public:
        /// create a context
        /// (see lsfg-vk documentation)
        ContextImpl(const InstanceImpl& instance,
            std::pair<int, int> sourceFds, const std::vector<int>& destFds, int syncFd,
            VkExtent2D extent, bool hdr, float flow, bool perf);

        /// schedule frames
        /// (see lsfg-vk documentation)
        void scheduleFrames();
    private:
        std::pair<vk::Image, vk::Image> sourceImages;
        std::vector<vk::Image> destImages;

        vk::TimelineSemaphore syncSemaphore; // imported
        vk::TimelineSemaphore prepassSemaphore;
        size_t idx{1};

        std::vector<vk::CommandBuffer> cmdbufs; // TODO: ponder reuse
        size_t cmdbuf_idx{0};

        ls::Ctx ctx;
    };
}

Instance::Instance(
        const std::function<bool(const std::string&)>& devicePicker,
        const std::filesystem::path& shaderDllPath,
        bool allowLowPrecision) {
    const auto selectFunc = [&devicePicker](const std::vector<VkPhysicalDevice>& devices) {
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            std::array<char, 256> devname = std::to_array(props.deviceName);
            devname[255] = '\0'; // ensure null-termination

            const std::string& deviceName{devname.data()};
            if (devicePicker(deviceName))
                return device;
        }

        throw ls::vulkan_error("no suitable physical device found");
    };

    this->m_impl = std::make_unique<InstanceImpl>(
        selectFunc, shaderDllPath, allowLowPrecision
    );
}

namespace {
    /// create a Vulkan instance
    vk::Vulkan createVulkanInstance(vk::PhysicalDeviceSelector selectPhysicalDevice) {
        try {
            return{
                "lsfg-vk", vk::version{1, 1, 0},
                "lsfg-vk-engine", vk::version{1, 1, 0},
                selectPhysicalDevice
            };
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to initialize Vulkan", e);
        }
    }
    /// build a shader registry
    extr::ShaderRegistry createShaderRegistry(vk::Vulkan& vk,
            const std::filesystem::path& shaderDllPath,
            bool allowLowPrecision) {
        std::unordered_map<uint32_t, std::vector<uint8_t>> resources{};

        try {
            resources = extr::extractResourcesFromDLL(shaderDllPath);
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to parse Lossless Scaling DLL", e);
        }

        try {
            return extr::buildShaderRegistry(
                vk, allowLowPrecision && vk.supportsFP16(),
                resources
            );
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to build shader registry", e);
        }
    }
#ifdef LSFGVK__RENDERDOC_INTEGRATION
    /// load RenderDoc integration
    std::optional<RENDERDOC_API_1_6_0> loadRenderDocIntegration() {
        void* module = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
        if (!module)
            return std::nullopt;

        auto renderdocGetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(
            dlsym(module, "RENDERDOC_GetAPI"));
        if (!renderdocGetAPI)
            return std::nullopt;

        RENDERDOC_API_1_6_0* api{};
        renderdocGetAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&api));
        if (!api)
            return std::nullopt;

        return *api;
    }
#endif
}

InstanceImpl::InstanceImpl(vk::PhysicalDeviceSelector selectPhysicalDevice,
            const std::filesystem::path& shaderDllPath,
            bool allowLowPrecision)
        : vk(createVulkanInstance(selectPhysicalDevice)),
        shaders(createShaderRegistry(this->vk, shaderDllPath,
            allowLowPrecision && vk.supportsFP16())) {
#ifdef LSFGVK__RENDERDOC_INTEGRATION
    this->renderdoc = loadRenderDocIntegration();
#endif
}

Context& Instance::openContext(std::pair<int, int> sourceFds, const std::vector<int>& destFds,
        int syncFd, uint32_t width, uint32_t height,
        bool hdr, float flow, bool perf) {
    const VkExtent2D extent{ width, height };
    return *this->m_contexts.emplace_back(std::make_unique<ContextImpl>(*this->m_impl,
        sourceFds, destFds, syncFd,
        extent, hdr, flow, perf
    )).get();
}

namespace {
    /// import source images
    std::pair<vk::Image, vk::Image> importImages(const vk::Vulkan& vk,
            const std::pair<int, int>& sourceFds,
            VkExtent2D extent, VkFormat format) {
        try {
            return {
                vk::Image(vk, extent, format,
                    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, sourceFds.first),
                vk::Image(vk, extent, format,
                    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, sourceFds.second)
            };
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to import destination images", e);
        }
    }
    /// import destination images
    std::vector<vk::Image> importImages(const vk::Vulkan& vk,
            const std::vector<int>& destFds,
            VkExtent2D extent, VkFormat format) {
        try {
            std::vector<vk::Image> destImages;
            destImages.reserve(destFds.size());

            for (const auto& fd : destFds)
                destImages.emplace_back(vk, extent, format,
                    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, fd);

            return destImages;
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to import destination images", e);
        }
    }
    /// import timeline semaphore
    vk::TimelineSemaphore importTimelineSemaphore(const vk::Vulkan& vk, int syncFd) {
        try {
            return{vk, 0, syncFd};
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to import timeline semaphore", e);
        }
    }
    /// create prepass semaphores
    vk::TimelineSemaphore createPrepassSemaphore(const vk::Vulkan& vk) {
        try {
            return{vk, 0};
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to create prepass semaphore", e);
        }
    }
    /// create command buffers
    std::vector<vk::CommandBuffer> createCommandBuffers(const vk::Vulkan& vk, size_t count) {
        try {
            std::vector<vk::CommandBuffer> cmdbufs;
            cmdbufs.reserve(count);

            for (size_t i = 0; i < count; ++i)
                cmdbufs.emplace_back(vk);

            return cmdbufs;
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to create command buffers", e);
        }
    }
    /// create context data
    ls::Ctx createCtx(const InstanceImpl& instance, VkExtent2D extent,
            bool hdr, float flow, bool perf, size_t count) {
        const auto& vk = instance.getVulkan();
        const auto& shaders = instance.getShaderRegistry();

        try {
            return {
                .vk = std::ref(vk),
                .shaders = std::ref(shaders),
                .constantBuffer{vk,
                    ls::getDefaultConstantBuffer(0, 1,
                        hdr, flow, false, false)},
                .bnbSampler{vk, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, false},
                .bnwSampler{vk, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, true},
                .eabSampler{vk, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_COMPARE_OP_ALWAYS, false},
                .sourceExtent = extent,
                .flowExtent = VkExtent2D {
                    .width = static_cast<uint32_t>(static_cast<float>(extent.width) / flow),
                    .height = static_cast<uint32_t>(static_cast<float>(extent.height) / flow)
                },
                .hdr = hdr,
                .flow = flow,
                .perf = perf,
                .count = count
            };
        } catch (const std::exception& e) {
            throw lsfgvk::error("Unable to create context", e);
        }
    }
}

ContextImpl::ContextImpl(const InstanceImpl& instance,
            std::pair<int, int> sourceFds, const std::vector<int>& destFds, int syncFd,
            VkExtent2D extent, bool hdr, float flow, bool perf) :
        sourceImages(importImages(instance.getVulkan(), sourceFds,
            extent, hdr ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM)),
        destImages(importImages(instance.getVulkan(), destFds,
            extent, hdr ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM)),
        syncSemaphore(importTimelineSemaphore(instance.getVulkan(), syncFd)),
        prepassSemaphore(createPrepassSemaphore(instance.getVulkan())),
        cmdbufs(createCommandBuffers(instance.getVulkan(), 16)),
        ctx(createCtx(instance, extent, hdr, flow, perf, destFds.size())) {
    // initialize all images
    const vk::CommandBuffer cmdbuf{ctx.vk};
    // (...)
    cmdbuf.submit(ctx.vk); // wait for completion
}

void Instance::scheduleFrames(Context& context) {
#ifdef LSFGVK__RENDERDOC_INTEGRATION
    const auto& impl = this->m_impl;
    if (impl->getRenderDocAPI()) {
        impl->getRenderDocAPI()->StartFrameCapture(
            RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(impl->getVulkan().inst()),
            nullptr);
    }
#endif
    try {
        context.scheduleFrames();
    } catch (const std::exception& e) {
        throw lsfgvk::error("Unable to schedule frames", e);
    }
#ifdef LSFGVK__RENDERDOC_INTEGRATION
    if (impl->getRenderDocAPI()) {
        vkDeviceWaitIdle(impl->getVulkan().dev());
        impl->getRenderDocAPI()->EndFrameCapture(
            RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(impl->getVulkan().inst()),
            nullptr);
    }
#endif
}

void Context::scheduleFrames() {
    // schedule pre-pass
    vk::CommandBuffer& cmdbuf = this->cmdbufs.at(this->cmdbuf_idx++ % this->cmdbufs.size());
    cmdbuf = vk::CommandBuffer(this->ctx.vk);

    // (...)

    cmdbuf.submit(this->ctx.vk,
        this->syncSemaphore, this->idx,
        this->prepassSemaphore, this->idx
    );

    this->idx++;

    // schedule main passes
    for (size_t i = 0; i < this->destImages.size(); ++i) {
        vk::CommandBuffer& cmdbuf = this->cmdbufs.at(this->cmdbuf_idx++ % this->cmdbufs.size());
        cmdbuf = vk::CommandBuffer(this->ctx.vk);

        // (...)

        cmdbuf.submit(this->ctx.vk,
            this->prepassSemaphore, this->idx - 1,
            this->syncSemaphore, this->idx + i
        );
    }

    this->idx += this->destImages.size();
}

void Instance::closeContext(const Context& context) {
    auto it = std::ranges::find_if(this->m_contexts,
        [context = &context](const std::unique_ptr<ContextImpl>& ctx) {
            return ctx.get() == context;
        });
    if (it == this->m_contexts.end())
        throw lsfgvk::error("attempted to close unknown context",
            std::runtime_error("no such context"));

    vkDeviceWaitIdle(this->m_impl->getVulkan().dev());
    this->m_contexts.erase(it);
}

Instance::~Instance() = default;

error::error(const std::string& msg, const std::exception& inner)
    : std::runtime_error(msg + "\n- " + inner.what()) {}

error::~error() = default;
