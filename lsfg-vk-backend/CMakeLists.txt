set(BACKEND_SOURCES
    "src/extraction/dll_reader.cpp"
    "src/extraction/shader_registry.cpp"
    "src/helpers/managed_shader.cpp"
    "src/helpers/utils.cpp"
    "src/shaderchains/alpha0.cpp"
    "src/shaderchains/alpha1.cpp"
    "src/shaderchains/beta0.cpp"
    "src/shaderchains/beta1.cpp"
    "src/shaderchains/mipmaps.cpp"
    "src/lsfgvk.cpp")

add_library(lsfg-vk-backend STATIC ${BACKEND_SOURCES})

target_include_directories(lsfg-vk-backend
    PUBLIC include)

target_link_libraries(lsfg-vk-backend
    PUBLIC lsfg-vk-common)

set_target_properties(lsfg-vk-backend PROPERTIES
    CXX_VISIBILITY_PRESET hidden)
