# lsfg-vk
**Lossless Scaling** is a Windows-only program that features various algorithms for scaling and interpolating programs.

**lsfg-vk** is a Vulkan layer that hooks into Vulkan applications and generates additional frames using Lossless Scaling's frame generation algorithm.

>[!CAUTION]
> You are reading the README for the upcoming version 2.0 of lsfg-vk. For the stable version 1.x, [please read here](https://github.com/PancakeTAS/lsfg-vk/tree/ff1a0f72a7d6d08b84d58b7b4dc5f05c9f904f98)

## Installation
>[!TIP]
> If you are on a Steam Deck or similar handheld device, consider using the [Decky plugin for lsfg-vk](https://github.com/xXJSONDeruloXx/decky-lsfg-vk). This is an easy way to install and configure lsfg-vk on the Steam Deck.
> Please keep in mind that this is not officially supported and support queries should be directed to the plugin's repository and Discord server.

1. Before proceeding, please make sure you have [Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/) installed on Steam.
2. Head to the [GitHub Releases](https://github.com/PancakeTAS/lsfg-vk/releases) page and download the file named "lsfg-vk-2.0.0-x86_64.tar.xz".
3. Open a terminal in the folder where you downloaded the file and run the following command:
```bash
tar -xvf lsfg-vk-2.0.0-x86_64.tar.xz -C ~/.local
```
This will extract lsfg-vk to the `~/.local` folder. Please **keep track of the files that were extracted**, in case you wish to uninstall lsfg-vk later.

4. The graphical interface of lsfg-vk requires Qt6 and Qt6 Quick in order to run. If you do not have these installed, install the following packages:
```bash
sudo apt install qt6-qpa-plugins libqt6quick6 qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtquick-dialogs qml6-module-qtqml-workerscript qml6-module-qtquick-templates qml6-module-qt-labs-folderlistmodel # On Debian/Ubuntu-based systems
sudo pacman -S qt6-declarative qt6-base # On Arch-based systems
sudo dnf install qt6-qtdeclarative qt6-qtbase # On Fedora
```

5. (Optional) If you wish to use lsfg-vk within Flatpak applications, see the [Flatpak Guide](docs/Flatpak-Guide.md).

## Usage
In order to use lsfg-vk, you will need to configure it. This can be done using the included GUI application, or by manually editing the configuration.

### Graphical Configuration
Start 'lsfg-vk Configuration Window' from your application launcher, or run `~/.local/bin/lsfg-vk-ui` in a terminal:
- On the left side, you will see a list of profiles. Each profile has its own settings.
- All properties in the "Global Settings" section apply to all profiles.
  - Should Lossless Scaling be installed in a non-standard location, you can specify its path here.
- Select a profile and configure the "Profile Settings" section to your liking.
  - When editing the "Active In" list, you can add a program using the name of its executable (e.g., `Game.exe`, `mpv`).
- Please see the [documentation](docs/Configuration.md) for detailed information about each setting.
- Once you are done configuring, starting a program that matches one of your profiles will automatically apply your settings.

### Manual Configuration
The default configuration is located in `~/.config/lsfg-vk/conf.toml`. It will be created automatically when any Vulkan application is started with lsfg-vk.
- In the `[global]` section, you can specify where Lossless Scaling is installed, as well as other global settings.
- Each profile is defined in its own `[[profile]]` section.
- The `active_in` array/string defines which programs the profile applies to. You can add programs using the name of their executable (e.g., `Game.exe`, `mpv`).
- Please see the [documentation](docs/Configuration.md) for detailed information about each setting.
- Once you are done configuring, starting a program that matches one of your profiles will automatically apply your settings.

You can validate your configuration using `lsfg-vk-cli`:
```bash
~/.local/bin/lsfg-vk-cli validate
```

### Benchmarking Mode
You can run a frame generation benchmark using `lsfg-vk-cli`:
```bash
~/.local/bin/lsfg-vk-cli benchmark
```

By default, the benchmark will run for 10 seconds. Add `-h` to the command to see all available benchmarking options.

## Support and Troubleshooting
If you encounter issues or have any questions regarding lsfg-vk, please see the [Troubleshooting](docs/Troubleshooting.md) documentation page, or join the [Discord server](https://discord.gg/losslessscaling) for further support.
