## lsfg-vk-framegen
Lossless Scaling Frame Generation

This is a subproject of lsfg-vk and contains the dedicated Vulkan logic for generating frames.

The project is intentionally structured as a fully external project, such that it can be integrated into other applications.

### Interface

Interfacing with lsfg-vk-framegen is done via `lsfg_x_x.hpp` header. The internal Vulkan instance is created using `LSFG_X_X::initialize()` and requires a specific deviceUUID, as well as parts of the lsfg-vk configuration, including a function loading SPIR-V shaders by name. Cleanup is done via `LSFG_X_X::finalize()` after which `LSFG_X_X::initialize()` may be called again. Please note that the initialization process is expensive and may take a while. It is recommended to call this function once during the applications lifetime.

Once the format and extent of the requested images is determined, `LSFG_X_X::createContext()` should be called to initialize a frame generation context. The Vulkan images are created from backing memory, which is passed through the file descriptor arguments. A context can be destroyed using `LSFG_X_X::deleteContext()`.

Presenting the context can be done via `LSFG_X_X::presentContext()`. Before calling the function a second time, make sure the outgoing semaphores have been signaled.
