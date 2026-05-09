set(CLI_SOURCES
    "src/tools/benchmark.cpp"
    "src/tools/debug.cpp"
    "src/tools/validate.cpp"
    "src/main.cpp")

add_executable(lsfg-vk-cli ${CLI_SOURCES})

target_include_directories(lsfg-vk-cli SYSTEM
    PRIVATE thirdparty/include)

target_link_libraries(lsfg-vk-cli
    PUBLIC lsfg-vk-common
    PUBLIC lsfg-vk-backend)

target_compile_options(lsfg-vk-cli PRIVATE
    -Wno-unsafe-buffer-usage) # CLI parsing

install(TARGETS lsfg-vk-cli
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
