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
            flatpak override \
              --user \
              --filesystem="$HOME/.local" \
              --filesystem="$HOME/.config/lsfg-vk" \
              --filesystem="$DLL_ABSOLUTE_PATH" \
              "$flat"
            # set up symlinks for lsfg-vk files
            mkdir -p "$APP_DIR/lib"
            mkdir -p "$APP_DIR/config/vulkan/implicit_layer.d/"
            mkdir -p "$APP_DIR/.config/lsfg-vk/"
            ln -s "$HOME/.local/lib/liblsfg-vk.so" "$APP_DIR/lib/liblsfg-vk.so"
            ln -s "$HOME/.local/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json" "$APP_DIR/config/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json"
            ln -s "$HOME/.config/lsfg-vk/conf.toml" "$APP_DIR/.config/lsfg-vk/conf.toml"
            echo "Usage enabled successfully for $flat."
        fi
    done

}

# run function only if flatpak is present
if command -v flatpak &> /dev/null; then
    flatpak_enabler
fi
