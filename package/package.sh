#!/bin/bash
if [ -z "$VERSION" ]; then
    set -eux
    export VERSION="1.1.0"
    export ALPM_VERSION=$(git describe --long --tags | sed 's/\([^-]*-\)g/r\1/;s/-/./g' | sed "s/.\..\../$VERSION/")
    export DPKG_VERSION=$(git describe --long --tags | sed 's/\([^-]*-\)g/r\1/;s/-/./g' | sed "s/^v.\..\..\.r[1-9]\+./$VERSION~git$(date '+%Y%m%d')./")
    export RPM_VERSION=$(git describe --long --tags | sed 's/\([^-]*-\)g/r\1/;s/-/./g' | sed "s/v.\..\...r/$VERSION^/")
else
    set -eux
    export ALPM_VERSION="$VERSION"
    export DPKG_VERSION="$VERSION"
    export RPM_VERSION="$VERSION"
fi

# set permission bits
chmod 755 bin/lsfg-vk-ui
chmod 755 lib/liblsfg-vk.so
chmod 644 share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
chmod 644 share/applications/lsfg-vk-ui.desktop
chmod 644 share/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png

# build alpm package
echo "Building ALPM package..."

mkdir -pv alpm
envsubst < package/alpm.PKGINFO > alpm/.PKGINFO

mkdir -pv alpm/usr/{bin,lib,share/vulkan/implicit_layer.d,share/applications,share/icons/hicolor/256x256/apps}
cp -v bin/lsfg-vk-ui alpm/usr/bin/lsfg-vk-ui
cp -v lib/liblsfg-vk.so alpm/usr/lib/liblsfg-vk.so
cp -v share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json \
    alpm/usr/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
cp -v share/applications/lsfg-vk-ui.desktop \
    alpm/usr/share/applications/lsfg-vk-ui.desktop
cp -v share/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png \
    alpm/usr/share/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png

tar -cvzf "lsfg-vk-$VERSION.x86_64.tar.zst" -C alpm \
    .PKGINFO usr

# build dpkg package
echo "Building DEB package..."

mkdir -pv deb/DEBIAN
envsubst < package/dpkg.control > deb/DEBIAN/control

mkdir -pv deb/usr/{bin,lib,share/vulkan/implicit_layer.d,share/applications,share/icons/hicolor/256x256/apps}
cp -v bin/lsfg-vk-ui deb/usr/bin/lsfg-vk-ui
cp -v lib/liblsfg-vk.so deb/usr/lib/liblsfg-vk.so
cp -v share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json \
    deb/usr/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
cp -v share/applications/lsfg-vk-ui.desktop \
    deb/usr/share/applications/lsfg-vk-ui.desktop
cp -v share/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png \
    deb/usr/share/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png

dpkg-deb --root-owner-group --build deb "lsfg-vk-$VERSION.x86_64.deb"

# build rpm package
echo "Building RPM package..."

mkdir -pv rpm
envsubst < package/rpm.spec > rpm/lsfg-vk.spec

mkdir -pv rpm/SOURCES
cp -v bin/lsfg-vk-ui rpm/SOURCES
cp -v lib/liblsfg-vk.so rpm/SOURCES
cp -v share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json \
    rpm/SOURCES
cp -v share/applications/lsfg-vk-ui.desktop \
    rpm/SOURCES/lsfg-vk-ui.desktop
cp -v share/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png \
    rpm/SOURCES/gay.pancake.lsfg-vk-ui.png

rpmbuild -bb rpm/lsfg-vk.spec --define "_topdir $(pwd)/rpm"
mv -v "rpm/RPMS/x86_64/lsfg-vk-$RPM_VERSION-1.x86_64.rpm" "lsfg-vk-$VERSION.x86_64.rpm"

# cleanup
rm -rf alpm deb rpm
