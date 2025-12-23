set(BACKEND_SOURCES
    "src/extraction/dll_reader.cpp"
    "src/extraction/shader_registry.cpp"
    "src/helpers/limits.cpp"
    "src/helpers/managed_shader.cpp"
    "src/helpers/utils.cpp"
    "src/shaderchains/alpha0.cpp"
    "src/shaderchains/alpha1.cpp"
    "src/shaderchains/beta0.cpp"
    "src/shaderchains/beta1.cpp"
    "src/shaderchains/delta0.cpp"
    "src/shaderchains/delta1.cpp"
    "src/shaderchains/gamma0.cpp"
    "src/shaderchains/gamma1.cpp"
    "src/shaderchains/generate.cpp"
    "src/shaderchains/mipmaps.cpp"
    "src/lsfgvk.cpp")

add_library(lsfg-vk-backend STATIC ${BACKEND_SOURCES})

target_include_directories(lsfg-vk-backend
    PUBLIC include)

target_link_libraries(lsfg-vk-backend
    PUBLIC lsfg-vk-common)

set_target_properties(lsfg-vk-backend PROPERTIES
    CXX_VISIBILITY_PRESET hidden)

if(LSFGVK_INSTALL_DEVELOP)
    install(TARGETS lsfg-vk-backend
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    install(DIRECTORY "include/lsfg-vk-backend/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lsfg-vk-backend"
        FILES_MATCHING PATTERN "*.hpp")
endif()
