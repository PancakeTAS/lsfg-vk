set(BACKEND_SOURCES
    "src/modules/library/dll.cpp"
    "src/modules/library.cpp"
    "src/modules/pipeline.cpp"
    "src/utility/pipelines.cpp"
    "src/utility/vkhelper.cpp"
    "src/lsfgvk.cpp")

add_library(lsfg-vk-backend STATIC ${BACKEND_SOURCES})

target_include_directories(lsfg-vk-backend
    PUBLIC include
    PRIVATE src)

target_compile_options(lsfg-vk-backend PUBLIC
    $<$<CXX_COMPILER_ID:Clang>:-fconstexpr-steps=4290000000>
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=4290000000>
)

if(LSFGVK_INSTALL_LIBRARIES)
    install(TARGETS lsfg-vk-backend
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    install(DIRECTORY "include/lsfg-vk-backend/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lsfg-vk-backend"
        FILES_MATCHING PATTERN "*.hpp")
endif()
