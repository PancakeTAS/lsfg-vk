# Troubleshooting
This page documents common issues, known incompatibilities and contains a guide to help you create a helpful bug report.

Before reporting a bug, please read through the following sections to see if your issue is already addressed.

### Basic Troubleshooting Steps
If lsfg-vk does not seem to be doing *anything*:
- Ensure the game you are trying to run is using Vulkan (not OpenGL).
- Ensure you are running a 64-bit game (try `PROTON_USE_WOW64=1`, but if it doesn't work then you're out of luck).
- Install `vulkan-tools` and run `vulkaninfo | grep -i VK_LAYER_LSFGVK_frame_generation`.
  - If there is no output revisit the installation steps.
- Launch the game with the environment variable `VK_LOADER_DEBUG=layer` set.
  - Look for lines mentioning `VK_LAYER_LSFGVK_frame_generation` inbetween `<Loader>` and `<Device>`.
  - If you can't find any, try again using `LSFGVK_ENV=1`.
    - If it still doesn't show up, you may be running in flatpak.
    - If it does show up, then the `active_in` property of your profile is likely misconfigured. Reconfigure it, then try again without `LSFGVK_ENV=1`.
- Check for warnings/errors from lsfg-vk in the terminal/log output. These will often give clues as to what is going wrong.
- If there are no errors/warnings and you have gone through all above steps, then move onto the next section.

If lsfg-vk is loaded, but frame generation is not working:
- (When using `pacing_mode = none`): Disable VRR.
- (When using `pacing_mode = none`): Explicitly enable V-Sync in your game settings.
- (When using `pacing_mode = none` on Gamescope/SteamDeck): Set `ENABLE_GAMESCOPE_WSI=0`.
- (When using `pacing_mode = none` on Wayland): Disable tearing control & direct passthrough in your compositor
- (When using `pacing_mode = none` on Wayland): Try running in windowed mode.
- Disable in-game upscaling options (e.g. DLSS, FSR, etc).
- Disable other Vulkan layers (e.g. VkBasalt, MangoHud)

Should none of the above help, please proceed to the bug reporting section.

### Performance Overlays
If you are using performance overlays like Steam's built-in overlay, there is a good chance that they will not show the correct framerate.

This is a known limitation of Vulkan layers and without directly working with the overlay developers, there is little that can be done to fix this.

### Opening a Bug Report
When opening a bug report, please include the following information to help us diagnose and fix the issue:
- A detailed description of the issue you are experiencing.
- What system you are running on (OS, GPU, drivers, etc).
- The game you are trying to run (and through what platform, e.g. Steam Proton, native Linux, etc).
- The relevant section of your lsfg-vk configuration file.

Ideally, also include a log file with the environment variables `VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation` and `VK_LOADER_DEBUG=all` set. You might need to install the Vulkan validation layers package for your distribution to do this.

If you're running the game through Steam, the log file is located at `~/.steam/steam/logs/console-linux.txt`. Please clear it before launching the game to ensure it only contains relevant information.
