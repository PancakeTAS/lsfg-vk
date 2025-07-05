include(ExternalProject)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(BUILD_TYPE "release")
    set(STRIP_FLAG "--strip")
else()
    set(BUILD_TYPE "debug")
    set(STRIP_FLAG "")
endif()

ExternalProject_Add(dxvk_git
    GIT_REPOSITORY "https://github.com/doitsujin/dxvk"
    GIT_TAG "v2.6.2"
    UPDATE_DISCONNECTED true
    USES_TERMINAL_CONFIGURE true
    USES_TERMINAL_BUILD true
    BUILD_IN_SOURCE true
    CONFIGURE_COMMAND
        sed -i s/private://g
            src/dxvk/dxvk_shader.h &&
        CC=clang CXX=clang++ CFLAGS=-w CXXFLAGS=-w
        meson setup
            --buildtype ${BUILD_TYPE}
            --prefix <SOURCE_DIR>/build-native
            ${STRIP_FLAG}
            -Dbuild_id=false
            --force-fallback-for=libdisplay-info
            --wipe
            build
    BUILD_COMMAND
        ninja -C build install &&
        mv build/src/dxvk/libdxvk.a build/src/dxvk/libldxvk.a &&
        cd build/src/dxgi/libdxvk_dxgi.so.0.20602.p &&
        bash -c "ar rcs -o ../libdxgi.a *.o" &&
        cd ../../d3d11/libdxvk_d3d11.so.0.20602.p &&
        bash -c "ar rcs -o ../libd3d11.a *.o .*.o"
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(dxvk_git SOURCE_DIR)

add_library(dxvk INTERFACE)
add_dependencies(dxvk dxvk_git)

target_link_directories(dxvk
    INTERFACE ${SOURCE_DIR}/build/src/dxvk
    INTERFACE ${SOURCE_DIR}/build/src/dxbc
    INTERFACE ${SOURCE_DIR}/build/src/dxgi
    INTERFACE ${SOURCE_DIR}/build/src/d3d11
    INTERFACE ${SOURCE_DIR}/build/src/spirv
    INTERFACE ${SOURCE_DIR}/build/src/util)
target_include_directories(dxvk SYSTEM
    INTERFACE ${SOURCE_DIR}/build-native/include/dxvk
    INTERFACE ${SOURCE_DIR}/src
    INTERFACE ${SOURCE_DIR}/include/spirv/include)
target_link_libraries(dxvk INTERFACE
    -Wl,--start-group dxgi d3d11 util ldxvk dxbc spirv -Wl,--end-group)
