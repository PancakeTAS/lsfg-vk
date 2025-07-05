include(ExternalProject)

ExternalProject_Add(peparse_git
    GIT_REPOSITORY "https://github.com/trailofbits/pe-parse"
    GIT_TAG "v2.1.1"
    UPDATE_DISCONNECTED true
    USES_TERMINAL_CONFIGURE true
    USES_TERMINAL_BUILD true
    BUILD_IN_SOURCE true
    CONFIGURE_COMMAND
        cmake -S pe-parser-library -B build -G Ninja
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_C_COMPILER=clang
            -DCMAKE_CXX_COMPILER=clang++
            -DCMAKE_C_FLAGS=-fPIC
            -DCMAKE_CXX_FLAGS=-fPIC
    BUILD_COMMAND
        ninja -C build
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(peparse_git SOURCE_DIR)

add_library(peparse INTERFACE)
add_dependencies(peparse peparse_git)

target_link_directories(peparse
    INTERFACE ${SOURCE_DIR}/build)
target_include_directories(peparse SYSTEM
    INTERFACE ${SOURCE_DIR}/pe-parser-library/include)
target_link_libraries(peparse
    INTERFACE pe-parse)
