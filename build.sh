#!/bin/sh

Binaries=$1
Platform=$2
Mode=release
Arch=$3
Formats=txt
Languages=en

BUILD_DIR=${BUILD_DIR-`pwd`/../Build}
QT_VERSION=${QT_VERSION-4.7.1}
BOOST_VERSION=${BOOST_VERSION-1.45.0}
QT_DIR=${BUILD_DIR}/qt-${QT_VERSION}-${Platform}-${Arch}
BOOST_DIR=${BUILD_DIR}/boost-${BOOST_VERSION}-${Platform}-${Arch}
if [ -d ${QT_DIR} ]; then
  echo Using static QT ver ${QT_VERSION}
  export STATIC_QT_PATH=${QT_DIR}
else
  echo Warning: ${QT_DIR} does not exists
fi
if [ -d ${BOOST_DIR} ]; then
  echo Using static boost ver ${BOOST_VERSION}
  export STATIC_BOOST_PATH=${BOOST_DIR}
else
  echo Warning: ${BOOST_DIR} does not exists
fi

# checking for textator or assume that texts are correct
textator --version > /dev/null 2>&1 || touch text/*.cpp text/*.h

# get current build and vesion
echo "Updating"
svn up > /dev/null || (echo "Failed to update"; exit 1)

echo "Clearing"
rm -Rf bin/${Platform}/${Mode} lib/${Platform}/${Mode} obj/${Platform}/${Mode} || exit 1;

# adding additional platform properties if required
case ${Arch} in
    ppc | ppc64 | powerpc)
      test `grep cpu /proc/cpuinfo | uniq | cut -f 3 -d " "` = "altivec" && cxx_flags="-mabi=altivec -maltivec"
      test `grep cpu /proc/cpuinfo | uniq | cut -f 2 -d " " | sed -e 's/,//g'` = "PPC970MP" && cxx_flags="${cxx_flags} -mtune=970 -mcpu=970"
    ;;
    x86_64)
      cxx_flags="-m64"
      ld_flags="-m64"
    ;;
    i386 | i486 | i586 | i686)
      cxx_flags="-m32 -march=${Arch}"
      ld_flags="-m32"
    ;;
    *)
    ;;
esac

for Binary in ${Binaries}
do
echo "Building ${Binary} with mode=${Mode} platform=${Platform} arch=${Arch}"
time make -j `grep processor /proc/cpuinfo | wc -l` package ${Mode}=1 platform=${Platform} arch=${Arch} cxx_flags="${cxx_flags}" ld_flags="${ld_flags}" -C apps/${Binary} || exit 1;
done
echo Done
