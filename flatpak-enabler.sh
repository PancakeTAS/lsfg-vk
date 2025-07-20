#!/bin/bash
# set up function
flatpak_enabler () {

    # initialize variables with locations
    DLL_FIND=$(find $HOME -name Lossless.dll | head -n 1)
    DLL_ABSOLUTE_PATH=$(dirname "$(realpath "$DLL_FIND")")
    ESCAPED_DLL_PATH=$(printf '%s\n' "$DLL_ABSOLUTE_PATH" | sed 's/[&/\]/\\&/g')
    CONF_LOC="${HOME}/.config/lsfg-vk/conf.toml"
    # check if config exists, and if not, gets a model file
    if [ ! -f "$CONF_LOC" ]; then
        # make sure target dir exists
        mkdir -p ${HOME}/.config/lsfg-vk/
        wget https://raw.githubusercontent.com/PancakeTAS/lsfg-vk/refs/heads/develop/conf.toml
        mv conf.toml ${HOME}/.config/lsfg-vk/
    fi
    # register dll location in config file
    sed -i -E "s|^# dll = \".*\"|dll = \"$ESCAPED_DLL_PATH\"|" ${HOME}/.config/lsfg-vk/conf.toml
    # apply flatpak overrides -- Lutris has permission for /home, so won't need any, but still needs the symlinks
    _flatpaks=(com.heroicgameslauncher.hgl com.valvesoftware.Steam net.lutris.Lutris)
    for flat in "${_flatpaks[@]}"; do
        if flatpak list | grep -q "$flat"; then
            APP_DIR="$HOME/.var/app/$flat"
            # overrides for AUR/CachyOS packages
            if command -v pacman &> /dev/null && pacman -Qi lsfg-vk 2>/dev/null 1>&2; then
                flatpak override \
                  --user \
                  --filesystem="/usr/lib/liblsfg-vk.so:ro" \
                  --filesystem="/etc/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json:ro" \
                  --filesystem="xdg-config/lsfg-vk:ro" \
                  --filesystem="$DLL_ABSOLUTE_PATH:ro" \
                  "$flat"
            # overrides for install script
            elif [ -f "$HOME/.local/lib/liblsfg-vk.so" ] && [ -f "$HOME/.local/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json" ]; then
                flatpak override \
                  --user \
                  --filesystem="$HOME/.local/lib/liblsfg-vk.so:ro" \
                  --filesystem="xdg-data/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json:ro" \
                  --filesystem="xdg-config/lsfg-vk:ro" \
                  --filesystem="$DLL_ABSOLUTE_PATH:ro" \
                  "$flat"
            fi
            # set up directories for symlinks
            mkdir -p "$APP_DIR/lib"
            mkdir -p "$APP_DIR/config/vulkan/implicit_layer.d/"
            mkdir -p "$APP_DIR/.config/lsfg-vk/"
            # symlinks for AUR/CachyOS packages
            if command -v pacman &> /dev/null && pacman -Qi lsfg-vk 2>/dev/null 1>&2; then
                ln -sf "/usr/lib/liblsfg-vk.so" "$APP_DIR/lib/liblsfg-vk.so"
                ln -sf "/etc/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json" "$APP_DIR/config/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json"
                ln -sf "$HOME/.config/lsfg-vk/conf.toml" "$APP_DIR/.config/lsfg-vk/conf.toml"
            # symlinks for installation script -- elif so it only creates the symlinks if files exist at the expected locations
            elif [ -f "$HOME/.local/lib/liblsfg-vk.so" ] && [ -f "$HOME/.local/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json" ]; then
                ln -sf "$HOME/.local/lib/liblsfg-vk.so" "$APP_DIR/lib/liblsfg-vk.so"
                ln -sf "$HOME/.local/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json" "$APP_DIR/config/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json"
                ln -sf "$HOME/.config/lsfg-vk/conf.toml" "$APP_DIR/.config/lsfg-vk/conf.toml"
            fi
            echo "Usage enabled successfully for $flat."
        fi
    done

}

# run function only if flatpak is present
if command -v flatpak &> /dev/null; then
    flatpak_enabler
fi
