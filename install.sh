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
printf "Enter the number (1-5): "
read -r version_choice < /dev/tty

case "$version_choice" in
    1) DISTRO="archlinux"; DISTRO_PRETTY="Arch Linux" ;;
    2) DISTRO="debian"; DISTRO_PRETTY="Debian" ;;
    3) DISTRO="ubuntu"; DISTRO_PRETTY="Ubuntu" ;;
    4) DISTRO="fedora"; DISTRO_PRETTY="Fedora" ;;
    5) DISTRO="nixos"; DISTRO_PRETTY="NixOS"; USE_NIX=true ;;
    *) echo "Invalid choice."; exit 1 ;;
esac

ZIP_NAME="lsfg-vk_${DISTRO}.zip"
SHA_NAME="lsfg-vk_${DISTRO}.zip.sha"
SHA_FILE="$INSTALL_PATH/share/lsfg-vk.sha"

# get local and remote versions
LOCAL_HASH=$(test -f "$SHA_FILE" && cat "$SHA_FILE")
if [ "$USE_NIX" ]; then
    command -v nix >/dev/null 2>&1 || { echo "Error: nix command not found."; exit 1; }
    REMOTE_HASH=$(curl -fsSL "${NIX_FLAKE_REPO/github.com/api.github.com/repos}/releases/latest" | grep '"tag_name"' | cut -d '"' -f 4)
else
    REMOTE_HASH=$(curl -fsSL "$BASE_URL/$SHA_NAME")
fi
[ -z "$REMOTE_HASH" ] && { echo "Failed to fetch latest release."; exit 1; }

if [ "$REMOTE_HASH" != "$LOCAL_HASH" ]; then
    # prompt user for confirmation
    echo -n "Are you sure you want to install lsfg-vk ($REMOTE_HASH) for ${DISTRO_PRETTY}? (y/n) "
    read -r answer < /dev/tty

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
    else
        # download and install prebuilt lsfg-vk
        curl -fsSL -o "$TEMP_DIR/$ZIP_NAME" "$BASE_URL/$ZIP_NAME" || { echo "Failed to download lsfg-vk. Please check your internet connection."; rm -vrf "$TEMP_DIR"; exit 1; }

        cd "$INSTALL_PATH" && unzip -o "$TEMP_DIR/$ZIP_NAME" || { echo "Extraction failed. Is install path writable or unzip installed?"; rm -vrf "$TEMP_DIR"; exit 1; }
    fi
    rm -vrf "$TEMP_DIR"

    echo "$REMOTE_HASH" > "$SHA_FILE"

    echo "lsfg-vk for ${DISTRO_PRETTY} has been installed."
else
    echo "lsfg-vk is up to date."

    # offer to uninstall
    echo -n "Do you want to uninstall lsfg-vk? (y/n) "
    read -r uninstall_answer < /dev/tty
    if [ "$uninstall_answer" = "y" ]; then
        rm -v $INSTALL_PATH/lib/liblsfg-vk.so
        rm -v $INSTALL_PATH/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
        rm -v "$SHA_FILE"
        echo "Uninstallation completed."
    fi
fi
