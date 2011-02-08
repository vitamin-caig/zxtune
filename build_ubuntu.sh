#!/bin/sh

Arch=${1-`uname -m`}

echo "Updating"
svn up > /dev/null || (echo "Failed to update" && exit 1)

. make/platforms/setup_qt.sh || exit 1
. make/platforms/setup_boost.sh || exit 1
# checking for textator or assume that texts are correct
textator --version > /dev/null 2>&1 || export NO_TEXTATOR=1

Revision=0`svnversion | sed -r 's/:/_/g'`

BuildDir=Builds/Revision${Revision}_linux_${Arch}-ubuntu
echo "Building at ${BuildDir}"
rm -Rf ${BuildDir}
mkdir -p ${BuildDir}

cpus=`grep processor /proc/cpuinfo | wc -l`
# adding additional platform properties if required
case ${Arch} in
    ppc | ppc64 | powerpc)
      test `grep cpu /proc/cpuinfo | uniq | cut -f 3 -d " "` = "altivec" && cxx_flags="-mabi=altivec -maltivec"
      test `grep cpu /proc/cpuinfo | uniq | cut -f 2 -d " " | sed -e 's/,//g'` = "PPC970MP" && cxx_flags="${cxx_flags} -mtune=970 -mcpu=970"
      PkgArch=powerpc
    ;;
    x86_64)
      cxx_flags="-m64 -march=x86-64 -mtune=generic -mmmx"
      ld_flags="-m64"
      PkgArch=amd64
    ;;
    i386 | i486 | i586 | i686)
      cxx_flags="-m32 -march=${Arch} -mtune=generic -mmmx"
      ld_flags="-m32"
      PkgArch=i386
    ;;
    *)
    ;;
esac

makecmd="make platform=linux release=1 arch=${Arch}-ubuntu"
${makecmd} -j ${cpus} all \
defines="ZXTUNE_VERSION=rev${Revision} BUILD_PLATFORM=linux BUILD_ARCH=${Arch}-ubuntu" \
cxx_flags="${cxx_flags}" ld_flags="${ld_flags}" \
-C apps/zxtune-qt | gzip -9 -c > ${BuildDir}/build.log.gz

PkgDir=${BuildDir}/pkg
mkdir -p ${PkgDir}
DESTDIR=`pwd`/${PkgDir} ${makecmd} install -C apps/zxtune-qt > ${BuildDir}/install.log
mkdir -p ${PkgDir}/usr/share/doc/zxtune-qt
echo "\
ZXTune\n\
\n\
Copyright 2009-2011 Vitamin <vitamin.caig@gmail.com>\n\
\n\
Visit http://zxtune.googlecode.com to get new versions and report about errors.\n\
\n\
This software is distributed under GPLv3 license.\n"\
> ${PkgDir}/usr/share/doc/zxtune-qt/copyright
echo "\
zxtune-qt (r${Revision}) experimental; urgency=low\n\
\n\
  * See svn log for details\n\
 -- Vitamin <vitamin.caig@gmail.com>  "`date -R`\
> ${PkgDir}/usr/share/doc/zxtune-qt/changelog
gzip --best ${PkgDir}/usr/share/doc/zxtune-qt/changelog

PkgFiles=`find ${PkgDir} -type f`
DebDir=${PkgDir}/DEBIAN
mkdir -p ${DebDir}
md5sum ${PkgFiles} | sed 's| .*pkg\/| |' > ${DebDir}/md5sums
echo "\
Package: zxtune-qt\n\
Version: ${Revision}\n\
Architecture: ${PkgArch}\n\
Priority: optional\n\
Section: sound\n\
Maintainer: Vitamin <vitamin.caig@gmail.com>\n\
Depends: libc6, libboost-thread1.40.0, libqtgui4 (>= 4.6.0), libqtcore4 (>= 4.6.0), libasound2\n\
Description: QT4-based application based on ZXTune library\n\
 This package provides zxtune-qt application\n\
 used to play chiptunes from ZX Spectrum.\n"\
> ${DebDir}/control

fakeroot dpkg-deb --build ${PkgDir} ${BuildDir}/zxtune-qt_r${Revision}_${Arch}-ubuntu.deb
