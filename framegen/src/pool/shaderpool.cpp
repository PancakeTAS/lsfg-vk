#include "pool/shaderpool.hpp"
#include "core/shadermodule.hpp"
#include "core/device.hpp"
#include "core/pipeline.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

using namespace LSFG;
using namespace LSFG::Pool;

Core::ShaderModule ShaderPool::getShader(
        const Core::Device& device, const std::string& name,
        const std::vector<std::pair<size_t, VkDescriptorType>>& types) {
    auto it = shaders.find(name);
    if (it != shaders.end())
        return it->second;

    // grab the shader
    auto bytecode = this->source(name);
    if (bytecode.empty())
        throw std::runtime_error("Shader code is empty: " + name);

    // create the shader module
    Core::ShaderModule shader(device, bytecode, types);
    shaders[name] = shader;
    return shader;
}

Core::Pipeline ShaderPool::getPipeline(
        const Core::Device& device, const std::string& name) {
    auto it = pipelines.find(name);
    if (it != pipelines.end())
        return it->second;

    // grab the shader module
    auto shader = this->getShader(device, name, {});

    // create the pipeline
    Core::Pipeline pipeline(device, shader);
    pipelines[name] = pipeline;
    return pipeline;
}
