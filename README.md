# lsfg-vk
This project brings [Lossless Scaling's Frame Generation](https://store.steampowered.com/app/993090/Lossless_Scaling/) to Linux!

## How does it work?
LSFG is primarily written in DirectX 11 compute shaders, which means we can use DXVK to translate it into SPIR-V. The surrounding parts have been rewritten in plain Vulkan code in order to make LSFG run natively on Linux.
By specifying an `LD_PRELOAD`, lsfg-vk can place itself inbetween your game and Vulkan. That way it can fetch frames from the game and insert its own frames without any problems. (Beware of anticheats please!)

## Building, Installing and Running
In order to compile LSFG, make sure you have the following components installed on your system:
- Traditional build tools (+ sed, git)
- Clang compiler (this project does NOT compile easily with GCC)
- Vulkan header files
- CMake build system
- Meson build system (for DXVK)
- Ninja build system (backend for CMake)

Compiling lsfg-vk is relatively straight forward, as everything is neatly integrated into CMake:
```bash
$ cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PWD/release \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DCMAKE_CXX_CLANG_TIDY=""
$ cmake --build build
$ cmake --install build
```
(Make sure you change `$PWD/release` to the path you'd like to install this project to)

Next, you'll need to download Lossless Scaling from Steam. Switch to the `legacy_2.13` branch or download the corresponding depot.
Copy or note down the path of "Lossless.dll" from the game files.

Finally, let's actually start a program with frame generation enabled. I'm going to be using `vkcube` for this example:
```bash
LD_PRELOAD="/home/pancake/.lsfg-vk/liblsfg-vk.so" LSFG_DLL_PATH="/home/pancake/games/Lossless Scaling/Lossless.dll" LSFG_MULTIPLIER=4 vkcube
```
Make sure you adjust the paths. Let's examine each one:
- `LD_PRELOAD`: This is how you let the loader know that you want to load lsfg-vk before anything else. This HAS to be either an ABSOLUTE path, or a path starting with `./` to make it relative. Specify `liblsfg-vk.so` here.
- `LSFG_DLL_PATH`: Here you specify the Lossless.dll you downloaded from Steam. This is either an absolute path, or a relative path from where you are running the app.
- `LSFG_MULTIPLIER`: This is the multiplier you should be familiar with. Specify `2` for doubling the framerate, etc.

>[!WARNING]
> Unlike on Windows, LSFG_MULTIPLIER is heavily limited here (at the moment!). If your hardware can create 8 swapchain images, then setting LSFG_MULTIPLIER to 4 occupies 4 of those, leaving only 4 to the game. If the game requested 5 or more, it will crash.
