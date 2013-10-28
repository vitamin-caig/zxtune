# Maintainer: vitamin.caig@gmail.com
pkgname=zxtune-svn
pkgver=VERSION
pkgrel=1
pkgdesc="Portable toolkit for ZX-Spectrum music playing"
arch=('i686' 'x86_64')
url="http://zxtune.googlecode.com"
license=('GPL3')
depends=('boost-libs' 'gcc-libs' 'qt4')
optdepends=(
'alsa-lib: for ALSA output support'
'lame: for conversion to .mp3 format'
'libvorbis: for conversion to .ogg format'
'flac: for conversion to .flac format'
'curl: for accessing data via network schemes'
)
makedepends=('subversion' 'gcc' 'zip' 'boost')
provides=('zxtune')
options=(!strip !docs !libtool !emptydirs !zipman makeflags)
_svntrunk="http://zxtune.googlecode.com/svn/tags/ver3"
_svnmod="zxtune"
_options="release=1 platform=linux arch=${CARCH} packaging=archlinux distro=current"
_qt4_options="qt.includes=/usr/include/qt4 tools.uic=uic-qt4 tools.moc=moc-qt4 tools.rcc=rcc-qt4"
_target="apps/bundle"

build() {
  cd "${srcdir}"
  if [ -d ${_svnmod}/.svn ]; then
    (cd ${_svnmod} && svn up -r ${pkgver})
  else
    svn co ${_svntrunk} --config-dir ./ -r ${pkgver} ${_svnmod}
  fi
  msg "SVN checkout done or server timeout"

  msg "Starting make"
  rm -rf "${srcdir}/${_svnmod}-build"
  svn export "${srcdir}/${_svnmod}" "${srcdir}/${_svnmod}-build"
  
  cd "${srcdir}/${_svnmod}-build"
  make ${MAKEFLAGS} all ${_options} cxx_flags="${CXXFLAGS}" ld_flags="${LDFLAGS}" ${_qt4_options} defines="BUILD_VERSION=${pkgver}" -C ${_target}
}

package() {
  cd "${srcdir}/${_svnmod}-build"
  make DESTDIR="${pkgdir}" ${_options} install -C ${_target} >/dev/null
}
