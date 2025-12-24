# lsfg-vk
**Lossless Scaling** is a Windows-exclusive program featuring various algorithms for scaling and interpolating programs.

**lsfg-vk** is a Vulkan layer that hooks into Vulkan applications and generates additional frames using Lossless Scaling's frame generation algorithm.

>[!CAUTION]
>You are reading the README for the upcoming version 2.0 of lsfg-vk. For the stable version 1.x, [please read here](https://github.com/PancakeTAS/lsfg-vk/tree/ff1a0f72a7d6d08b84d58b7b4dc5f05c9f904f98)

More information, as well as how to build from source, can be found on the [wiki](https://pancake.gay/lsfg-vk/).

## Installation
1. Before proceeding, please make sure you have [Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/) downloaded on Steam.
2. Head to the [GitHub Releases](https://github.com/PancakeTAS/lsfg-vk/releases) and download the file named "lsfg-vk-2.0.0-x86_64.tar.xz".
3. Open a terminal in the folder where you downloaded the file and run the following:
```bash
tar -xvf lsfg-vk-2.0.0-linux.tar.xz -C ~/.local
```
This will extract lsfg-vk to `~/.local`. Please **keep track of the files that were extracted**, in case you want to uninstall lsfg-vk later.

4. The graphical interface requires Qt6 and Qt6 Quick in order to run. If you do not have these installed, install the following packages:
```bash
sudo apt install qt6-qpa-plugins libqt6quick6 # On Debian/Ubuntu-based systems
sudo pacman -S qt6-declarative qt6-base # On Arch-based systems
sudo dnf install qt6-qtdeclarative qt6-qtbase # On Fedora
```

5. (Optional) If you wish to use lsfg-vk within Flatpak applications, see the [Flatpak Setup Guide](https://pancake.gay/lsfg-vk/#/flatpak).

## Usage
In order to start using lsfg-vk, you will need to configure it. This can either be done using the GUI application, or manually.

### Graphical Configuration
Start 'lsfg-vk Configuration Window' from your application launcher, or run `~/.local/bin/lsfg-vk-ui` in a terminal:
- On the left side, you will see a list of profiles. Each profile has its own settings.
- All properties in the "Global Settings" section apply to all profiles.
  - Should Lossless Scaling be installed in a non-standard location, you can specify the path here.
- Select a profile and configure the "Profile Settings" section to your liking.
  - When editing the "Active In" list, you can add a game using its executable name (e.g. `Game.exe`, `mpv`).
- Please see the [wiki](https://pancake.gay/lsfg-vk/#/configure) for detailed information on each setting.
- Once you are done configuring, simply starting a game that matches one of the profiles will automatically apply the settings.

### Manual Configuration
The default configuration is located in `~/.config/lsfg-vk/conf.toml`. It will be created automatically when any Vulkan application is started.
- In the `[global]` section, you can change where Lossless Scaling is installed, as well as other global settings.
- Each profile is defined in its own `[[profile]]` section.
- The `active_in` array/string defines which applications the profile is active in. You can add applications using their executable name (e.g. `Game.exe`, `mpv`).
- Please see the [wiki](https://pancake.gay/lsfg-vk/#/configure) for detailed information on each setting.
- Once you are done configuring, simply starting a game that matches one of the profiles will automatically apply the settings.

You can validate the configuration using `lsfg-vk-cli`:
```bash
~/.local/bin/lsfg-vk-cli validate
```

### Environment Variables
Certain environment variables can be used to modify the behavior of lsfg-vk:
- `LSFGVK_CONFIG`: Specify a custom path to the configuration file.
- `LSFGVK_PROFILE`: Choose a specific profile to use by name, overriding the automatic selection.
- `LSFGVK_ENV`: Configure the profile based on environment variables. Please see the [wiki](https://pancake.gay/lsfg-vk/#/configure) for more information.

### Benchmarking Mode
You can run a frame generation benchmark using `lsfg-vk-cli`:
```bash
~/.local/bin/lsfg-vk-cli benchmark
```

By default, the benchmark will run for 10 seconds. Add `-h` to see all available benchmarking options.

## Support and Troubleshooting
If you encounter any issues or have questions regarding lsfg-vk, read through the [Troubleshooting](https://pancake.gay/lsfg-vk/#/troubleshooting) page on the wiki or join the [Discord server](https://discord.gg/losslessscaling) for assistance.
