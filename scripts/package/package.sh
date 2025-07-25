#!/bin/sh
if [ -z "$VERSION" ]; then
    echo "VERSION environment variable is not set."
    exit 1
fi

set -eux

# build alpm package
echo "Building ALPM package..."

mkdir -pv alpm
envsubst < scripts/package/alpm.PKGINFO > alpm/.PKGINFO

mkdir -pv alpm/usr/{bin,lib,share/vulkan/implicit_layer.d}
cp -v bin/lsfg-vk-ui alpm/usr/bin/lsfg-vk
cp -v lib/liblsfg-vk.so alpm/usr/lib/liblsfg-vk.so
cp -v share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json alpm/usr/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json

tar -cvzf "lsfg-vk-$VERSION.x86_64.tar.zst" -C alpm \
    .PKGINFO usr

# build dpkg package
echo "Building DEB package..."

mkdir -pv deb/DEBIAN
envsubst < scripts/package/dpkg.control > deb/DEBIAN/control

mkdir -pv deb/usr/{bin,lib,share/vulkan/implicit_layer.d}
cp -v bin/lsfg-vk-ui deb/usr/bin/lsfg-vk-ui
cp -v lib/liblsfg-vk.so deb/usr/lib/liblsfg-vk.so
cp -v share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json deb/usr/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json

dpkg-deb --root-owner-group --build deb "lsfg-vk-$VERSION.x86_64.deb"

# build rpm package
echo "Building RPM package..."

mkdir -pv rpm
envsubst < scripts/package/rpm.spec > rpm/lsfg-vk.spec

mkdir -pv rpm/SOURCES
cp -v bin/lsfg-vk-ui rpm/SOURCES
cp lib/liblsfg-vk.so rpm/SOURCES
cp share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json rpm/SOURCES

rpmbuild -bb rpm/lsfg-vk.spec --define "_topdir $(pwd)/rpm"
mv -v "rpm/RPMS/x86_64/lsfg-vk-$VERSION-1.x86_64.rpm" "lsfg-vk-$VERSION.x86_64.rpm"

# cleanup
rm -rf alpm deb rpm
