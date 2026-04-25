/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace lsfgvk {

    /// Forward declaration of implementation classes
    namespace priv {
        struct [[gnu::visibility("default")]] Instance;
        struct [[gnu::visibility("default")]] Context;
    }

    ///
    /// Main entrypoint of the library
    ///
    class [[gnu::visibility("default")]] Instance {
        friend class Context;
    public:
        ///
        /// Create a lsfg-vk instance
        ///
        /// The device identifier may be one of:
        /// - Device name (e.g. "NVIDIA GeForce RTX 5080")
        /// - Vendor ID + Device ID in lowercase hexadecimal (e.g. "10de:2c02")
        /// - PCI bus ID with padded zeroes (e.g. "0000:01:00.0")
        ///
        /// @param deviceId Device identifier (see above)
        /// @param lsfgvkDllPath Path to the lsfg-vk DLL file
        /// @param allowFP16 Whether to allow usage of fp16 shader variants
        /// @throws std::runtime_error on failure
        ///
        Instance(
            const std::string& deviceId,
            const std::filesystem::path& lsfgvkDllPath,
            bool allowFP16
        );

        // Non-copyable, non-movable
        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;
        Instance(Instance&&) = delete;
        Instance& operator=(Instance&&) = delete;
        ~Instance();
    private:
        std::unique_ptr<priv::Instance> m_priv;
    };

    ///
    /// File descriptors exported from a context, the user must close them after use.
    ///
    struct FileDescriptors {
        ///
        /// File descriptor for a Vulkan memory allocation containing
        /// a 2D array of RGBA8 pixels with length 2 and optimal allocation.
        ///
        /// Starting at iteration 0, the next frame for which frames should be interpolated
        /// inbetween should be placed in image `iteration % 2`.
        ///
        int sourceFd;

        ///
        /// File descriptor for a Vulkan memory allocation containing a single RGBA8
        /// image into which each generated frame will be written to.
        ///
        int destinationFd;

        ///
        /// File descriptor for a timeline semaphore. When scheduling frames for generation,
        /// a specific value is waited for and signaled on return. It is up to the user to ensure
        /// the destination image is not overwritten before it is read.
        ///
        int syncFd;
    };

    /// A context for generating frames
    ///
    class [[gnu::visibility("default")]] Context {
    public:
        ///
        /// Create a frame generation context
        ///
        /// @param instance Parent instance
        /// @param width Image width
        /// @param height Image height
        /// @param flowScale Flow estimation scale factor
        /// @param performanceMode Whether to enable performance mode
        /// @throws std::runtime_error on failure
        ///
        Context(
            const Instance& instance,
            uint32_t width,
            uint32_t height,
            float flowScale,
            bool performanceMode
        );

        ///
        /// Export the internal resources
        ///
        /// @return File descriptors for internal resources
        /// @throws std::runtime_error on failure
        ///
        [[nodiscard]] FileDescriptors exportFds() const;

        ///
        /// Dispatch frame generation
        ///
        /// Let `so - 1` be the current value of the timeline semaphore, starting at 0.
        /// The user must signal `so` to start the generation of the next frame, after
        /// which lsfg-vk will signal `so + 1`. The user must ensure the previously
        /// generated frame is read before signaling the next one (at `so + 2` and so on).
        ///
        /// @param total Total number of frames to generate
        /// @throws std::runtime_error on failure
        ///
        void dispatch(uint32_t total);

        ///
        /// Wait for the device to be idle
        ///
        void idle() const;

        // Non-copyable, non-movable
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context();
    private:
        std::unique_ptr<priv::Context> m_priv;
    };

}
