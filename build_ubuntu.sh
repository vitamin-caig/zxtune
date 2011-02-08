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
      arch=powerpc
    ;;
    x86_64)
      cxx_flags="-m64 -march=x86-64 -mtune=generic -mmmx"
      ld_flags="-m64"
      arch=amd64
    ;;
    i386 | i486 | i586 | i686)
      cxx_flags="-m32 -march=${Arch} -mtune=generic -mmmx"
      ld_flags="-m32"
      arch=i386
    ;;
    *)
    ;;
esac

makecmd="make platform=linux release=1 arch=${arch}-ubuntu"
${makecmd} -j ${cpus} all \
defines="ZXTUNE_VERSION=rev${Revision} BUILD_PLATFORM=linux BUILD_ARCH=${arch}-ubuntu" \
cxx_flags="${cxx_flags}" ld_flags="${ld_flags}" \
-C apps/zxtune-qt | gzip -9 -c > ${BuildDir}/build.log.gz

PkgDir=${BuildDir}/pkg
DebDir=${PkgDir}/DEBIAN
mkdir -p ${DebDir}
DESTDIR=`pwd`/${PkgDir} ${makecmd} install -C apps/zxtune-qt > ${BuildDir}/install.log
PkgFiles=`find ${PkgDir} -type f | grep -v '^${DebDir}/'`
touch ${DebDir}/changelog
echo 7 > ${DebDir}/compat
md5sum ${PkgFiles} > ${DebDir}/md5sums
echo "\
Package: zxtune-qt\n\
Version: r${Revision}\n\
Architecture: ${arch}\n\
Priority: optional\n\
Section: sound\n\
Description: QT4-based application based on ZXTune library\n\
Maintainer: vitamin.caig@gmail.com\n\
Depends: libboost-thread1.40.0, libqtgui4 (>= 4.6.0), libqtcore4 (>= 4.6.0), libasound2\n\
Installed-Size: "`du -cs ${PkgFiles} | tail -n 1 | cut -f1`"\n" > ${DebDir}/control

dpkg-deb -b ${PkgDir} ${BuildDir}/zxtune-qt_r${Revision}_${Arch}-ubuntu.deb
