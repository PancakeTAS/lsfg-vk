#!/bin/sh

: "${INSTALL_PATH:=$HOME/.local}"
BASE_URL='https://pancake.gay/lsfg-vk'
NIX_FLAKE_REPO='https://github.com/pabloaul/lsfg-vk-flake'

# prompt for distro
echo "Which version would you like to install?"
echo "1) Arch Linux (Artix Linux, CachyOS, Steam Deck, etc.)"
echo "2) Debian"
echo "3) Ubuntu"
echo "4) Fedora"
echo "5) NixOS (external flake project)"
echo "6) Flatpak Runtime (23.08)"
echo "7) Flatpak Runtime (24.08)"
printf "Enter the number (1-7): "
read -r version_choice < /dev/tty

case "$version_choice" in
    1) DISTRO="archlinux"; DISTRO_PRETTY="Arch Linux" ;;
    2) DISTRO="debian"; DISTRO_PRETTY="Debian" ;;
    3) DISTRO="ubuntu"; DISTRO_PRETTY="Ubuntu" ;;
    4) DISTRO="fedora"; DISTRO_PRETTY="Fedora" ;;
    5) DISTRO="nixos"; DISTRO_PRETTY="NixOS"; USE_NIX=true ;;
    6) DISTRO="23.08"; DISTRO_PRETTY="Flatpak Runtime 23.08" ;;
    7) DISTRO="24.08"; DISTRO_PRETTY="Flatpak Runtime 24.08" ;;
    *) echo "Invalid choice."; exit 1 ;;
esac

ZIP_NAME="lsfg-vk_${DISTRO}.zip"
SHA_NAME="lsfg-vk_${DISTRO}.zip.sha"
if [ "$DISTRO" = "23.08" ]; then
    SHA_FILE="$INSTALL_PATH/share/lsfg-vk-2308.sha"
elif [ "$DISTRO" = "24.08" ]; then
    SHA_FILE="$INSTALL_PATH/share/lsfg-vk-2408.sha"
else
    SHA_FILE="$INSTALL_PATH/share/lsfg-vk.sha"
fi

# offer installing UI
if [ "$DISTRO" != "23.08" ] && [ "$DISTRO" != "24.08" ]; then
    printf 'Do you want to install or update the user interface as well? (y/n) '
    read answer < /dev/tty
    if [ "$answer" = "y" ]; then
        UI_ZIP="lsfg-vk_ui.zip"
        UI_SHA="lsfg-vk_ui.zip.sha"
        UI_SHA_FILE="$INSTALL_PATH/share/lsfg-vk-ui.sha"
        LOCAL_UI_HASH=$(test -f "$UI_SHA_FILE" && cat "$UI_SHA_FILE")
        REMOTE_UI_HASH=$(curl -fsSL "$BASE_URL/$UI_SHA")
        [ -z "$REMOTE_UI_HASH" ] && { echo "Failed to fetch latest release."; exit 1; }
        if [ "$REMOTE_UI_HASH" != "$LOCAL_UI_HASH" ]; then
            UI_INSTALL="1"
        else
            UI_INSTALL="0"
        fi
    fi
fi


# get local and remote versions
LOCAL_HASH=$(test -f "$SHA_FILE" && cat "$SHA_FILE")
if [ "$USE_NIX" ]; then
    command -v nix >/dev/null 2>&1 || { echo "Error: nix command not found."; exit 1; }
    API_URL=$(printf '%s' "$NIX_FLAKE_REPO" | sed 's|github.com|api.github.com/repos|')
    REMOTE_HASH=$(curl -fsSL "$API_URL/releases/latest" | grep '"tag_name"' | cut -d '"' -f 4)
else
    REMOTE_HASH=$(curl -fsSL "$BASE_URL/$SHA_NAME")
fi
[ -z "$REMOTE_HASH" ] && { echo "Failed to fetch latest release."; exit 1; }

if [ "$REMOTE_HASH" != "$LOCAL_HASH" ]; then
    # prompt user for confirmation
    printf 'Are you sure you want to install lsfg-vk (%s) for %s? (y/n) ' "$REMOTE_HASH" "$DISTRO_PRETTY"
    read answer < /dev/tty

    if [ "$answer" != "y" ]; then
        echo "Installation aborted."
        exit 0
    fi

    TEMP_DIR=$(mktemp -d) && cd "$TEMP_DIR" || { echo "Failed to create temporary directory."; exit 1; }
    if [ "$USE_NIX" ]; then
        # download, build and install lsfg-vk-flake from GitHub
        curl -fsSL "$NIX_FLAKE_REPO/archive/refs/tags/$REMOTE_HASH.tar.gz" | tar -xz

        cd lsfg-vk-flake-* && nix build || { echo "Build failed."; rm -vrf "$TEMP_DIR"; exit 1; }

        install -Dvm644 result/lib/liblsfg-vk.so "$INSTALL_PATH/lib/liblsfg-vk.so"
        install -Dvm644 result/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json "$INSTALL_PATH/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json"

    elif [ "$DISTRO" = "23.08" ] || [ "$DISTRO" = "24.08" ]; then
        # install flatpak runtimes
        if command -v flatpak >/dev/null 2>&1; then
            echo "Reminder: you need to have lsfg-vk installed on the host to enable flatpak runtimes."
            sleep 3

            curl -fsSL -o "$TEMP_DIR/$ZIP_NAME" "$BASE_URL/$ZIP_NAME" || { echo "Failed to download lsfg-vk. Please check your internet connection."; rm -vrf "$TEMP_DIR"; exit 1; }

            unzip "$TEMP_DIR/$ZIP_NAME" || { echo "Extraction failed. Is install path writable or unzip installed?"; rm -vrf "$TEMP_DIR"; exit 1; }

            flatpak install --user --bundle --noninteractive "$TEMP_DIR"/org.freedesktop.Platform.VulkanLayer.lsfg_vk_${DISTRO}.flatpak
        else
            echo "Flatpak not available in the system, installation aborted."
            exit 0
        fi

    else
        # download and install prebuilt lsfg-vk
        curl -fsSL -o "$TEMP_DIR/$ZIP_NAME" "$BASE_URL/$ZIP_NAME" || { echo "Failed to download lsfg-vk. Please check your internet connection."; rm -vrf "$TEMP_DIR"; exit 1; }

        cd "$INSTALL_PATH" && unzip -o "$TEMP_DIR/$ZIP_NAME" || { echo "Extraction failed. Is install path writable or unzip installed?"; rm -vrf "$TEMP_DIR"; exit 1; }
    fi

    if [ "$UI_INSTALL" = "1" ]; then
        curl -fsSL -o "$TEMP_DIR/$UI_ZIP" "$BASE_URL/$UI_ZIP" || { echo "Failed to download lsfg-vk. Please check your internet connection."; rm -vrf "$TEMP_DIR"; exit 1; }

        cd "$HOME" && unzip -o "$TEMP_DIR/$UI_ZIP" || { echo "Extraction failed. Is install path writable or unzip installed?"; rm -vrf "$TEMP_DIR"; exit 1; }

        echo "$REMOTE_UI_HASH" > "$UI_SHA_FILE"
    fi

    rm -vrf "$TEMP_DIR"

    echo "$REMOTE_HASH" > "$SHA_FILE"

    echo "lsfg-vk for ${DISTRO_PRETTY} has been installed."

else
    echo "lsfg-vk is up to date."

    if [ "$UI_INSTALL" = "1" ]; then
        curl -fsSL -o "$TEMP_DIR/$UI_ZIP" "$BASE_URL/$UI_ZIP" || { echo "Failed to download lsfg-vk. Please check your internet connection."; rm -vrf "$TEMP_DIR"; exit 1; }

        cd "$HOME" && unzip -o "$TEMP_DIR/$UI_ZIP" || { echo "Extraction failed. Is install path writable or unzip installed?"; rm -vrf "$TEMP_DIR"; exit 1; }

        echo "$REMOTE_UI_HASH" > "$UI_SHA_FILE"

        echo "lsfg-vk UI has been installed."
        exit 0
    fi

    # offer to uninstall
    printf 'Do you want to uninstall lsfg-vk? (y/n) '
    read -r uninstall_answer < /dev/tty
    if [ "$uninstall_answer" = "y" ]; then
        rm -v $INSTALL_PATH/lib/liblsfg-vk.so
        rm -v $INSTALL_PATH/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
        rm -v "$SHA_FILE"
        if [ "$UI_INSTALL" = "0" ]; then
            rm -v "$UI_SHA_FILE"
        fi
        echo "Uninstallation completed."
    fi
fi
