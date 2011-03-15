#!/bin/sh

Arch=${1-`uname -m`}

echo "Updating"
svn up > /dev/null || (echo "Failed to update" && exit 1)
# checking for textator or assume that texts are correct
textator --version > /dev/null 2>&1 || export NO_TEXTATOR=1
BOOST_DEPENDENCY="boost-libs="`pacman -Q boost | sed -nr 's/boost (.*)-.*/\1/pg'`
QT_DEPENDENCY="qt>=4.5.0"

Revision=`svnversion | sed -r 's/:/_/g'`

CurDir=`pwd`
BuildDir=Builds/Revision${Revision}_linux_${Arch}-archlinux
echo "Building ar ${BuildDir}"
rm -Rf ${BuildDir}
mkdir -p ${BuildDir}

DEPENDENCIES="'${BOOST_DEPENDENCY}'"
for App in zxtune-qt zxtune123
do
  echo "Generating PKGBUILD for ${App} revision ${Revision} arch ${Arch}"

echo -e "\
# Maintainer: vitamin.caig@gmail.com\n\
pkgname=${App}\n\
pkgver=r${Revision}\n\
pkgrel=1\n\
pkgdesc=\"ZX Spectrum sound chiptunes playback support\"\n\
arch=('${Arch}')\n\
url=\"http://zxtune.googlecode.com\"\n\
license=('GPL3')\n\
depends=(${DEPENDENCIES})\n\
provides=('${App}')\n\
options=(!strip !docs !libtool !emptydirs !zipman makeflags)\n\
_options=\"release=1 platform=linux arch=\${CARCH}-archlinux\"\n\
\n\
build() {\n\
  msg \"Starting make\"\n\
  make \${MAKEFLAGS} all \${_options} cxx_flags=\"\${CXXFLAGS}\" ld_flags=\"\${LDFLAGS}\"\
 defines=\"ZXTUNE_VERSION=rev${Revision} BUILD_PLATFORM=linux BUILD_ARCH=\${CARCH}-archlinux\" -C ${CurDir}/apps/${App} |\
 gzip -9 -c > ${CurDir}/${BuildDir}/build_${App}.log.gz\n\
}\n\
\n\
package() {\n\
  make DESTDIR=\"\${pkgdir}\" \${_options} install -C ${CurDir}/apps/${App} >/dev/null\n\
}\n" > ${BuildDir}/PKGBUILD.${App}

(cd ${BuildDir} && makepkg -c -p PKGBUILD.${App}) || echo "Failed"

DEPENDENCIES+=" '${QT_DEPENDENCY}'"
done
