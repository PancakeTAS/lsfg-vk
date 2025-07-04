# Maintainer: Placeholder <placeholder@mail.com>

pkgname=lsfg-vk
pkgver=r75.9611a70
pkgrel=1
pkgdesc="A modern Vulkan renderer for LSFG"
arch=('x86_64')
url="https://github.com/PancakeTAS/lsfg-vk"
license=('MIT')
depends=('vulkan-headers' 'spirv-headers' 'openssl' 'clang' 'meson')
makedepends=('git' 'cmake' 'ninja')
provides=('lsfg-vk')
source=("git+$url.git")
md5sums=('SKIP')

pkgver() {
  cd "$srcdir/$pkgname"
  printf "r%s.%s" \
    "$(git rev-list --count HEAD)" \
    "$(git rev-parse --short HEAD)"
}

build() {
  cd "$srcdir/$pkgname"
  git submodule update --init --recursive

  CC=clang CXX=clang++ cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$pkgdir/usr"

  cmake --build build
}

package() {
  cd "$srcdir/$pkgname"
  cmake --install build
}
