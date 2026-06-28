# Flatpak Guide

If you want to use **lsfg-vk** with Flatpak applications, you must install the Vulkan layer for Flatpak. You can also optionally install the graphical configuration editor **lsfg-vk-ui** as a Flatpak application.

## Installation

There are three different ways of installing lsfg-vk for Flatpak.

### 1. Through Flathub

The main lsfg-vk layer is available on Flathub and can be installed with the following commands (omit `--user` for a system-wide installation):
```bash
flatpak install --user org.freedesktop.Platform.VulkanLayer.lsfgvk//24.08
flatpak install --user org.freedesktop.Platform.VulkanLayer.lsfgvk//25.08
```

If you require an older runtime (23.08), you must install it manually as shown in the next section. Similarly, if you want to install the graphical configuration editor **lsfg-vk-ui**, you must also install it manually.

### 2. Through GitHub Releases

Head over to the lsfg-vk [GitHub Releases](https://github.com/PancakeTAS/lsfg-vk/releases) page and download the Flatpak files for the Vulkan layer and/or the graphical configuration editor.

It can be installed with the following commands (omit`--user` for a system-wide installation):
```bash
tar -xf lsfg-vk-2.0.0-flatpaks-x86_64.tar.xz
flatpak --user install ./org.freedesktop.Platform.VulkanLayer.lsfg-vk-23.08.flatpak
flatpak --user install ./org.freedesktop.Platform.VulkanLayer.lsfg-vk-24.08.flatpak
flatpak --user install ./org.freedesktop.Platform.VulkanLayer.lsfg-vk-25.08.flatpak
flatpak --user install ./gay.pancake.lsfg-vk-ui.flatpak
```

You can then run the graphical configuration editor with:
```bash
flatpak run gay.pancake.lsfg_vk_ui
```

### 3. Through Custom Build

If you want to build lsfg-vk yourself, install `flatpak-builder` and run the following commands:
```bash
git clone --depth=1 https://github.com/PancakeTAS/lsfg-vk.git
# optional: git checkout <desired-version>
cd lsfg-vk
flatpak-builder --force-clean --user --install-deps-from=flathub --install flatpak-build \
    dist/flatpak/lsfg-vk-ui/gay.pancake.lsfg-vk-ui.yml
flatpak-builder --force-clean --user --install-deps-from=flathub --install flatpak-build \
    dist/flatpak/lsfg-vk-layer/org.freedesktop.Platform.VulkanLayer.lsfgvk_23.08.yml
flatpak-builder --force-clean --user --install-deps-from=flathub --install flatpak-build \
    dist/flatpak/lsfg-vk-layer/org.freedesktop.Platform.VulkanLayer.lsfgvk_24.08.yml
flatpak-builder --force-clean --user --install-deps-from=flathub --install flatpak-build \
    dist/flatpak/lsfg-vk-layer/org.freedesktop.Platform.VulkanLayer.lsfgvk_25.08.yml
```

## Configuration

Before using lsfg-vk with Flatpak applications, you need to give them access to the configuration directory, as well as to Lossless Scaling.
```bash
export appid=  # e.g., io.mpv.Mpv
mkdir -p ~/.config/lsfg-vk
flatpak override --user --filesystem=/home/$USER/.config/lsfg-vk:rw $appid
flatpak override --user --filesystem=/home/$USER/local/share/Steam/steamapps/common:ro $appid
flatpak override --user --env=LSFGVK_CONFIG=/home/$USER/.config/lsfg-vk/conf.toml $appid
```