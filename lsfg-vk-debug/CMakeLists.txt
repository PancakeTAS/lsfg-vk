set(DEBUG_SOURCES
    "src/debug.cpp")

add_executable(lsfg-vk-debug ${DEBUG_SOURCES})

target_link_libraries(lsfg-vk-debug
    PUBLIC lsfg-vk-common
    PUBLIC lsfg-vk-backend)
