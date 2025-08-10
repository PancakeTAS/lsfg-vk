# lsfg-vk

**lsfg-vk** brings Lossless Scaling's frame generation feature to Linux users. Lossless Scaling is a Windows-exclusive application that offers various algorithms for scaling and interpolating windows, and **lsfg-vk** acts as a Vulkan layer to enable this functionality between your game and graphics card on Linux.

**Important:** To use **lsfg-vk**, you must own Lossless Scaling on Steam. You can purchase it here: [Lossless Scaling on Steam](https://store.steampowered.com/app/993090/Lossless_Scaling).

## Features

* **Vulkan Layer Implementation:** Seamlessly injects frames into Vulkan applications.
* **Linux Compatibility:** Designed to work across various Linux distributions.
* **User Interface:** Includes **lsfg-vk-ui** for easy configuration (note: not included with Flatpak installations).

## Getting Started

For comprehensive instructions on setting up, configuring, and using **lsfg-vk**, please refer to our detailed [Wiki](https://github.com/PancakeTAS/lsfg-vk/wiki) guides.

### Installation

**lsfg-vk** is available as pre-built packages for various Linux distributions.

1. **Download:** Visit the [Releases page](https://github.com/PancakeTAS/lsfg-vk/releases) to download the package for your Linux distribution.
2. **Install:** Follow the step-by-step instructions in the [Installation Guide](https://github.com/PancakeTAS/lsfg-vk/wiki/Installation-Guide) for your specific distro (Debian, RPM, ZST, Generic .zip, Flatpak).

**Note for older versions:** If you had **lsfg-vk** installed user-wide (prior to version 0.9.0), please delete the old installation files before proceeding.

### Configuration and Usage

After installation, you can open the **lsfg-vk** Configuration Window from your application menu or by typing `lsfg-vk-ui` in your console.

* **Configure:** For detailed instructions on setting up your preferences, visit the [Configuring lsfg-vk](https://github.com/PancakeTAS/lsfg-vk/wiki/Configuring-lsfg%E2%80%90vk) page.
* **Verify Installation:** To confirm that **lsfg-vk** is correctly set up, refer to the [Verifying the installation](https://github.com/PancakeTAS/lsfg-vk/wiki/Verifying-the-installation) guide.
* **Integrated Benchmark:** Learn how to use **lsfg-vk**'s integrated benchmark on the [Using lsfg-vk's integrated benchmark](https://github.com/PancakeTAS/lsfg-vk/wiki/Using-lsfg%E2%80%90vk's-integrated-benchmark) page.

## Building from Source

If you prefer to build **lsfg-vk** yourself for development, debugging, or to use the latest source, a detailed build guide is available in the [Build Guide](https://github.com/PancakeTAS/lsfg-vk/wiki/Build-Guide) page.

## Support and Troubleshooting

If you encounter any issues or need assistance, please consult the following resources:

* **How to Ask for Help:** For proper troubleshooting steps and reporting instructions, refer to the [How to ask for help](https://github.com/PancakeTAS/lsfg-vk/wiki/How-to-ask-for-help) page.
* **Discord:** Join the [Lossless Scaling Discord server](https://discord.gg/losslessscaling) for help (Steam verification required).
* **Known Issues:** Check the [Known incompatibilities](https://github.com/PancakeTAS/lsfg-vk/wiki/Known-incompatibilities) page for a list of known issues.

## Credits

Most of the project has still only been written by me, PancakeTAS, but I couldn't have done it without the help of these people:

* **0xNULLderef:** Teaching me how to reverse engineer software.
* **Caliel666:** Writing the initial draft of the user interface.
* **Samueru-sama:** Helping with various things XDG as well as app images and testing.
* Other contributors: Thank you for your contributions!

I'd also like to thank every single person sponsoring this project. Thanks to you I'll be able to invest more time into this and hopefully bring some cool new features to everyone.

## More Information

For deeper insights into how **lsfg-vk** works:

* [Porting LSFG to native Vulkan](https://github.com/PancakeTAS/lsfg-vk/wiki/Porting-LSFG-to-native-Vulkan)
* [Injecting frames into Vulkan apps](https://github.com/PancakeTAS/lsfg-vk/wiki/Injecting-frames-into-Vulkan-apps)
