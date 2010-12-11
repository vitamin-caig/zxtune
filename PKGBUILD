# Contributor: vitamin.caig@gmail.com
pkgname=zxtune
pkgver=r`svnversion`
pkgrel=1
pkgdesc="ZX Spectrum sound chiptunes playback support"
arch=('i686' 'x86_64')
url="http://zxtune.googlecode.com"
license=('GPL3')
depends=('boost-libs>=1.40' 'qt>=4.5.0')
_options="release=1 platform=linux arch=${CARCH}"

build() {
  make clean ${_options} -C ${startdir}/apps || exit 1
  make -j `grep processor /proc/cpuinfo | wc -l` ${_options} defines="ZXTUNE_VERSION=${pkgver} BUILD_PLATFORM=linux BUILD_ARCH=${CARCH}" -C ${startdir}/apps || exit 1
  make DESTDIR="$pkgdir" ${_options} install -C ${startdir}/apps || exit 1
}
