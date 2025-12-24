set(CLI_SOURCES
    "src/tools/benchmark.cpp"
    "src/tools/debug.cpp"
    "src/tools/validate.cpp"
    "src/main.cpp")

add_executable(lsfg-vk-cli ${CLI_SOURCES})

target_link_libraries(lsfg-vk-cli
    PUBLIC lsfg-vk-common
    PUBLIC lsfg-vk-backend)

target_compile_options(lsfg-vk-cli PRIVATE
    -Wno-unknown-warning-option
    -Wno-unsafe-buffer-usage)

install(TARGETS lsfg-vk-cli
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
