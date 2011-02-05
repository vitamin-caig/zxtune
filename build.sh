#!/bin/sh

Platform=$1
Arch=$2
Mode=release
Formats=txt
Languages=en

BUILD_DIR=${BUILD_DIR-`pwd`/../Build}
QT_VERSION=${QT_VERSION-4.7.1}
BOOST_VERSION=${BOOST_VERSION-1.45.0}
QT_DIR=${BUILD_DIR}/qt-${QT_VERSION}-${Platform}-${Arch}
BOOST_DIR=${BUILD_DIR}/boost-${BOOST_VERSION}-${Platform}-${Arch}

# Detect QT
if [ -d "${QT_DIR}" ]; then
  echo Using static QT ver ${QT_VERSION}
  export STATIC_QT_PATH=${QT_DIR}
else
  QT_HEADERS_DIR=`qmake -query QT_INSTALL_HEADERS 2> /dev/null`
  if [ "x${QT_HEADERS_DIR}" != "x" ]; then
    echo Using QT headers in ${QT_HEADERS_DIR}
    export CPLUS_INCLUDE_PATH="${CPLUS_INCLUDE_PATH}:${QT_HEADERS_DIR}"
  else
    echo Error: unable to detect path to QT headers
    exit 1
  fi
  QT_LIBS_DIR=`qmake -query QT_INSTALL_LIBS 2> /dev/null`
  if [ "x${QT_LIBS_DIR}" != "x" ]; then
    echo Using QT libs in ${QT_LIBS_DIR}
    export LIBRARY_PATH="${LIBRARY_PATH}:${QT_LIBS_DIR}"
  else
    echo Error: unable to detect path to QT libs
    exit 1
  fi
fi
# Detect boost
if [ -d "${BOOST_DIR}" ]; then
  echo Using static boost ver ${BOOST_VERSION}
  export STATIC_BOOST_PATH=${BOOST_DIR}
else
  BOOST_DIR=/usr/include/boost
  if [ -d "${BOOST_DIR}" ]; then
    echo Using boost in ${BOOST_DIR}
  else
    echo Error: ${BOOST_DIR} does not exists
    exit 1
  fi
fi

cpus=`grep processor /proc/cpuinfo | wc -l`
makecmd="make platform=${Platform} ${Mode}=1 arch=${Arch} -C apps"

# checking for textator or assume that texts are correct
textator --version > /dev/null 2>&1 || export NO_TEXTATOR=1 && echo "No textator used"

# get current build and vesion
echo "Updating"
svn up > /dev/null || (echo "Failed to update"; exit 1)

echo "Clearing"
${makecmd} clean > /dev/null || exit 1

# adding additional platform properties if required
case ${Arch} in
    ppc | ppc64 | powerpc)
      test `grep cpu /proc/cpuinfo | uniq | cut -f 3 -d " "` = "altivec" && cxx_flags="-mabi=altivec -maltivec"
      test `grep cpu /proc/cpuinfo | uniq | cut -f 2 -d " " | sed -e 's/,//g'` = "PPC970MP" && cxx_flags="${cxx_flags} -mtune=970 -mcpu=970"
    ;;
    x86_64)
      cxx_flags="-m64 -march=x86-64 -mtune=generic -mmmx"
      ld_flags="-m64"
    ;;
    i386 | i486 | i586 | i686)
      cxx_flags="-m32 -march=${Arch} -mtune=generic -mmmx"
      ld_flags="-m32"
    ;;
    *)
    ;;
esac

if [ "x${STATIC_QT_PATH}y${STATIC_BOOST_PATH}" != "xy" ]; then
echo Using static runtime
options="static_runtime=1"
fi

time ${makecmd} -j ${cpus} package ${options} cxx_flags="${cxx_flags}" ld_flags="${ld_flags}" && echo Done
