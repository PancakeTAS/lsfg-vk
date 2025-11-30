set(BACKEND_SOURCES
    "src/extraction/dll_reader.cpp"
    "src/extraction/shader_registry.cpp"
    "src/helpers/managed_shader.cpp")

add_library(lsfg-vk-backend STATIC ${BACKEND_SOURCES})

target_include_directories(lsfg-vk-backend
    PUBLIC include)

target_link_libraries(lsfg-vk-backend
    PUBLIC lsfg-vk-common)

set_target_properties(lsfg-vk-backend PROPERTIES
    CXX_VISIBILITY_PRESET hidden)
