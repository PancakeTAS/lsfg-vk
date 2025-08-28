#!/bin/sh
set -eux

# AppImage build script provided by @Samueru-sama
# (with removed aarch64 supported and removed update functionality, temporarily)

URUNTIME="https://github.com/VHSgunzo/uruntime/releases/latest/download/uruntime-appimage-dwarfs-x86_64"
URUNTIME_LITE="https://github.com/VHSgunzo/uruntime/releases/latest/download/uruntime-appimage-dwarfs-lite-x86_64"
SHARUN="https://github.com/VHSgunzo/sharun/releases/latest/download/sharun-x86_64-aio"

VERSION=$(awk -F'=|"' '/^version/{print $3}' ./Cargo.toml)
echo "$VERSION-dev" > ~/version

# grab required resources
wget -O sharun-aio "$SHARUN"
chmod +x sharun-aio

wget -O uruntime "$URUNTIME"
wget -O uruntime-lite "$URUNTIME_LITE"
chmod +x uruntime uruntime-lite

# build lsfg-vk-ui
echo "Building lsfg-vk-ui..."
cargo build --release

# deploy app directory
echo "Deploying app directory..."
mkdir -p AppDir
cp -v rsc/*.desktop AppDir
cp -v rsc/icon.png AppDir/lsfg-vk-ui.png
cp -v rsc/icon.png AppDir/.DirIcon
mkdir -p AppDir/shared/bin
mv -v target/release/lsfg-vk-ui AppDir/shared/bin

cd AppDir
    xvfb-run -a ../sharun-aio l -p -v -e -s -k \
        shared/bin/lsfg-vk-ui \
        /usr/lib/*/libdecor* \
        /usr/lib/*/gdk-pixbuf-*/*/loaders/* \
        /usr/lib/*/gio/modules/libdconfsettings.so
    ln -fv ./sharun ./AppRun
    ./sharun -g
cd ..

# create appimage
echo "Generating app image..."
./uruntime --appimage-mkdwarfs -f \
	--set-owner 0 --set-group 0 \
	--no-history --no-create-timestamp \
	--compression zstd:level=22 -S26 -B8 \
	--header uruntime-lite \
	-i ./AppDir -o "./lsfg-vk-ui.AppImage"

# cleanup
echo "Cleaning up..."
rm -rf AppDir
rm -f sharun-aio uruntime uruntime-lite

# done
echo "Done"
