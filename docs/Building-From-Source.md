# Building lsfg-vk From Source
This guide provides step-by-step instructions on how to build the lsfg-vk project from source code.

>[!IMPORTANT]
>If you are planning on compiling lsfg-vk on a Steam Deck, you need to disable the read-only protection and reinstall certain system packages first:
> ```bash
> sudo steamos-readonly disable
> sudo pacman-key --init
> sudo pacman-key --populate
> sudo pacman -Syy
> sudo pacman -S linux-headers linux-api-headers glibc

### Prerequisites
Before you begin, ensure you have the required packages installed on your system.

You will need the following dependencies:
- Common build tools, such as `git`, `curl`, etc.
- A C++ compiler that supports the C++20 standard or later
- CMake (version 3.10 or higher)
- Ninja build system (other build systems may work, but Ninja is recommended)
- Vulkan SDK
- Qt6 and Qt6Quick (only needed when building lsfg-vk-ui)

The list of required packages may vary depending on your operating system. Below are the installation commands for some common Linux distributions:
```bash
# On Debian/Ubuntu, use:
sudo apt-get install -y \
    git curl \
    llvm clang clang-tools clang-tidy \
    cmake ninja-build pkg-config \
    libvulkan-dev \
    mesa-common-dev \
    qt6-base-dev qt6-base-dev-tools \
    qt6-tools-dev qt6-tools-dev-tools \
    qt6-declarative-dev qt6-declarative-dev-tools

# On Arch Linux, use:
sudo pacman -S --needed \
    git curl \
    llvm clang \
    cmake ninja \
    vulkan-headers vulkan-icd-loader \
    qt6-base qt6-declarative
```

### Building and Installing lsfg-vk

1. **Clone the Repository**

Clone the lsfg-vk repository from GitHub:
```bash
git clone https://github.com/PancakeTAS/lsfg-vk.git
cd lsfg-vk
```

Optionally, you can checkout a specific release tag:
```bash
git checkout tags/vX.Y.Z
```

2. **Configure the Build With CMake**

The recommended way to configure lsfg-vk is:
```bash
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLSFGVK_BUILD_UI=On \
    -DLSFGVK_INSTALL_XDG_FILES=On
```

However, lsfg-vk provides several CMake options to customize the build process:
- `CMAKE_BUILD_TYPE`: Set to `Release` for optimized builds or `Debug` to build for debugging.
- `CMAKE_INSTALL_PREFIX`: Specify the installation directory (default is `/usr/local`).
- `LSFGVK_BUILD_VK_LAYER`: Set to `On` to build the Vulkan layer (default is `On`).
- `LSFGVK_BUILD_UI`: Set to `On` to build the graphical user interface (default is `Off`).
- `LSFGVK_BUILD_CLI`: Set to `On` to build the command-line interface (default is `On`).
- `LSFGVK_INSTALL_DEVELOP`: Set to `On` to install development files like headers and libraries (default is `Off`).
- `LSFGVK_INSTALL_XDG_FILES`: Set to `On` to install XDG desktop files and icons (default is `Off`).
- `LSFGVK_LAYER_LIBRARY_PATH`: Override the path to the Vulkan layer library (by default, Vulkan will search the systems library path).

Please keep in mind that installing to non-system paths will require `LSFGVK_LAYER_LIBRARY_PATH` to be set accordingly (e.g. `../../../lib/liblsfg-vk-layer.so`).

3. **Build the Project**

Build the project using Ninja:
```bash
cmake --build build
```

4. **Install the Project**

Install the built files to the specified installation prefix:
```bash
sudo cmake --install build
```

Keep track of installed files in case you wish to uninstall them later.